local str2addr = function (s)
	local a = { s:match("(%d+)%.(%d+)%.(%d+)%.(%d+)") }
	local n = tonumber(s:match("/(%d+)"))
	local r = { n = n }
	for i = 1, n do
		r[i] = math.floor(a[math.floor((i - 1) / 8) + 1] / math.pow(2, 7 - (i - 1) % 8)) % 2
	end
	return r
end

local addr2str = function (a)
	local sa = { 0, 0, 0, 0 }
	local idx
	for i = 1, a.n do
		idx = math.floor((i - 1) / 8) + 1
		sa[idx] = sa[idx] * 2 + a[i]
	end
	sa[idx] = sa[idx] * math.pow(2, 7 - (a.n - 1) % 8)
	return sa[1] .. "." .. sa[2] .. "." .. sa[3] .. "." .. sa[4] .. "/" .. a.n
end

local tadd
tadd = function (t, p, i)
	if i == p.n then
		t[p[i]] = true
	elseif type(t[p[i]]) == "nil" then
		t[p[i]] = {}
		return tadd(t[p[i]], p, i + 1)
	elseif type(t[p[i]]) == "table" then
		tadd(t[p[i]], p, i + 1)
		if t[p[i]][0] == true and t[p[i]][1] == true then
			t[p[i]] = true
		end
	end
end

local tlist
tlist = function (t, p, cb)
	if type(t) ~= "table" then
		if type(t) ~= "nil" then
			return cb(t, p)
		end
		return
	end
	p.n = p.n + 1
	p[p.n] = 0
	tlist(t[0], p, cb)
	p[p.n] = 1
	tlist(t[1], p, cb)
	p.n = p.n - 1
end

local add = function (this, s)
	local a = str2addr(s)
	return tadd(this.tree, a, 1)
end

local list = function (this, s)
	local r = {}
	local cb = function (t, p)
		table.insert(r, addr2str(p))
	end
	local p = { n = 0 }
	tlist(this.tree, p, cb)
	return r
end

local create = function (class)
	return {
		add = class.add,
		list = class.list,
		tree = {}
	}
end

return {
	add = add,
	list = list,
	create = create
}
