all: chn_ipv4_list.txt

data-ASnet-detail!
	wget -N 'http://thyme.apnic.net/current/data-ASnet-detail'

delegated-apnic-latest!
	wget -N 'http://ftp.apnic.net/apnic/stats/apnic/delegated-apnic-latest'

chn_ipv4_list.txt: data-ASnet-detail delegated-apnic-latest
	./genlist.lua > '$@'
