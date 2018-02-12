#!/usr/bin/lua

ipset = require("ipset")

function genASNset()
	local s = {n = 0}
	for l in io.lines("delegated-apnic-latest") do
		local asn, num = l:match("^apnic%|CN%|asn%|(%d+)%|(%d+)%|")
		if asn then
			asn = tonumber(asn)
			num = tonumber(num)
			for _ = 1, num do
				s[asn] = true
				s.n = s.n + 1
				asn = asn + 1
			end
		end
	end
	return s
end

function genList(asnset, ips)
	local prn = false
	for l in io.lines("data-ASnet-detail") do
		local asn, net = l:match("^(%d+)"), l:match("^%s+([%d./]+)")
		if asn then
			if asnset[tonumber(asn)] then
				prn = true
			else
				prn = false
			end
		end
		if prn and net then
			ips:add(net)
		end
	end
end

function main()
	local as = genASNset()
	local ips = ipset:create()
	genList(as, ips)
	for _, v in pairs(ips:list()) do
		print(v)
	end
end

main()
