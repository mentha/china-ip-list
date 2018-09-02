#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

#include <cstdint>
#include <cstdio>
#include <cstring>

using namespace std;

struct Prefix {
	int width;
	int leng;
	uint32_t prefix;
	void set(int width, uint32_t prefix, int leng) {
		this->width = width;
		this->leng = leng;
		if (leng == width)
			this->prefix = prefix;
		else
			this->prefix = (prefix & ~((uint32_t) 0xffffffffU >> leng));
		this->prefix &= (uint32_t) 0xffffffffU >> (32 - width);
	};
	Prefix(int width, uint32_t prefix, int leng) {
		set(width, prefix, leng);
	};
	Prefix(string s) {
		unsigned a, b, c, d;
		int l;
		if (sscanf(s.c_str(), "%u.%u.%u.%u/%d",
					&a, &b, &c, &d, &l) != 5)
			throw runtime_error("prefix parse failed");
		set(32, a << 24 | b << 16 | c << 8 | d, l);
	};
	void prepend(int bit) {
		prefix |= (bit & 0x1) << width;
		width++;
		leng++;
	};
	int slice() {
		width--;
		leng--;
		int r = (prefix >> width) & 0x1;
		prefix &= (uint32_t) 0xffffffffU >> (32 - width);
		return r;
	};
	string format() {
		if (width != 32)
			throw runtime_error("trying to format partial prefix");
		char buf[100];
		snprintf(buf, 100, "%u.%u.%u.%u/%d",
				(prefix >> 24) & 0xff,
				(prefix >> 16) & 0xff,
				(prefix >> 8) & 0xff,
				prefix & 0xff, leng);
		return string(buf);
	};
};

struct PrefixSet {
	int width;
	bool empty;
	bool uni;
	PrefixSet *zero;
	PrefixSet *one;
	PrefixSet(int width = 32) {
		this->width = width;
		empty = true;
		uni = false;
		zero = nullptr;
		one = nullptr;
	};
	PrefixSet(Prefix p) {
		width = p.width;
		if (p.leng == 0) {
			empty = false;
			uni = true;
			zero = nullptr;
			one = nullptr;
		} else {
			empty = false;
			uni = false;
			if (p.slice() == 1) {
				zero = new PrefixSet(p.width);
				one = new PrefixSet(p);
			} else {
				zero = new PrefixSet(p);
				one = new PrefixSet(p.width);
			}
		}
	};
	PrefixSet &operator= (const PrefixSet &p) {
		if (!empty && !uni) {
			delete zero;
			delete one;
		};
		width = p.width;
		empty = p.empty;
		uni = p.uni;
		if (empty || uni) {
			zero = nullptr;
			one = nullptr;
		} else {
			zero = new PrefixSet(*p.zero);
			one = new PrefixSet(*p.one);
		}
		return *this;
	};
	PrefixSet(const PrefixSet &p) {
		uni = true;
		*this = p;
	};
	~PrefixSet() {
		if (!empty && !uni) {
			delete zero;
			delete one;
		};
	};
	void neg() {
		if (empty) {
			empty = false;
			uni = true;
			return;
		}
		if (uni) {
			empty = true;
			uni = false;
			return;
		}
		zero->neg();
		one->neg();
	};
	void merge(PrefixSet &n) {
		if (uni or n.empty)
			return;
		if (n.uni or empty) {
			*this = n;
			return;
		};
		zero->merge(*n.zero);
		one->merge(*n.one);
		if (zero->uni && one->uni) {
			delete zero;
			delete one;
			zero = nullptr;
			one = nullptr;
			uni = true;
		}
	};
	void merge(Prefix n) {
		auto p = PrefixSet(n);
		this->merge(p);
	};
	void remove(PrefixSet &n) {
		if (empty || n.empty)
			return;
		if (n.uni) {
			if (!uni) {
				delete zero;
				delete one;
				zero = nullptr;
				one = nullptr;
			}
			uni = false;
			empty = true;
			return;
		}
		if (uni) {
			*this = n;
			this->neg();
			return;
		}
		zero->remove(*n.zero);
		one->remove(*n.one);
		if (zero->empty && one->empty) {
			delete zero;
			delete one;
			zero = nullptr;
			one = nullptr;
			empty = true;
		}
	};
	void remove(Prefix n) {
		auto p = PrefixSet(n);
		this->remove(p);
	};
	void iter(function<void(Prefix)> cb) {
		if (uni) {
			cb(Prefix(width, 0, 0));
			return;
		}
		if (empty)
			return;
		zero->iter([cb](Prefix p) {
				p.prepend(0);
				cb(p);
				});
		one->iter([cb](Prefix p) {
				p.prepend(1);
				cb(p);
				});
	};
};

set<int> load_asn()
{
	fstream f("delegated-apnic-latest", ios_base::in);
	set<int> r;
	while (!f.eof()) {
		char l[1000];
		f.getline(l, 1000);
		if (l[0] == '#')
			continue;
		char *p = strchr(l, '|');
		if (p == NULL)
			continue;
		p += 1;
		if (memcmp(p, "CN|", 3))
			continue;
		p += 3;
		if (memcmp(p, "asn|", 4))
			continue;
		p += 4;
		int asn, num;
		if (sscanf(p, "%d", &asn) != 1)
			continue;
		p = strchr(p, '|');
		if (p == NULL)
			continue;
		p += 1;
		if (sscanf(p, "%d", &num) != 1)
			continue;
		for (; num > 0; num--) {
			r.insert(asn);
			asn++;
		}
	}
	return r;
}

PrefixSet load_announce(set<int> cnasn)
{
	fstream f("data-ASnet-detail", ios_base::in);
	auto s = PrefixSet();
	int cur = -1;
	while (!f.eof()) {
		char l[1000];
		f.getline(l, 1000);
		if (l[0] == 0)
			continue;
		if (l[0] >= '0' && l[0] <= '9') {
			if (sscanf(l, "%d", &cur) != 1)
				cur = -1;
			if (cnasn.find(cur) == cnasn.end())
				cur = -1;
			continue;
		}
		if (cur != -1 && strchr(l, '/') != NULL) {
			char *p = strpbrk(l, "1234567890");
			if (p == NULL)
				continue;
			auto pfx = Prefix(p);
			s.merge(pfx);
		}
	}
	return s;
}

int main(void)
{
	auto cnasn = load_asn();
	auto l = load_announce(cnasn);
	fstream chnip("chn_ipv4_list.txt", ios_base::out);
	l.iter([&chnip] (Prefix pfx) {
			chnip << pfx.format() << endl;
			});
	l.neg();
	l.remove(Prefix("0.0.0.0/8"));
	l.remove(Prefix("10.0.0.0/8"));
	l.remove(Prefix("100.64.0.0/10"));
	l.remove(Prefix("127.0.0.0/8"));
	l.remove(Prefix("169.254.0.0/16"));
	l.remove(Prefix("172.16.0.0/12"));
	l.remove(Prefix("192.0.0.0/24"));
	l.remove(Prefix("192.0.2.0/24"));
	l.remove(Prefix("192.88.99.0/24"));
	l.remove(Prefix("192.168.0.0/16"));
	l.remove(Prefix("198.18.0.0/15"));
	l.remove(Prefix("198.51.100.0/24"));
	l.remove(Prefix("203.0.113.0/24"));
	l.remove(Prefix("224.0.0.0/4"));
	l.remove(Prefix("240.0.0.0/4"));
	fstream ncip("non_chn_ipv4_list.txt", ios_base::out);
	l.iter([&ncip] (Prefix pfx) {
			ncip << pfx.format() << endl;
			});
	return 0;
}
