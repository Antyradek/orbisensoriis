-- orbisensoriis protocol example
orbisensoriis_proto = Proto("orbisensoriis","Orbisensoriis Protocol")
-- create a function to dissect it
function orbisensoriis_proto.dissector(buffer,pinfo,tree)
    pinfo.cols.protocol = "ORBISENSORIIS"
    local subtree = tree:add(orbisensoriis_proto,buffer(),"Orbisensoriis Protocol Data")
    local msg_type = bit.band(buffer(0,1):uint(), 0x7)

    if msg_type == 1 then 
        subtree:add(buffer(0,1),"INIT_MSG")
        local timeout = bit.band(bit.rshift(buffer(0,4):uint(),3), 0x3FFF)
        local period = bit.band(bit.rshift(buffer(0,4):uint(),17), 0x3FFF)
        subtree:add(buffer(0,1),"timeout: " .. timeout)
        subtree:add(buffer(0,1),"period: " .. period)
    elseif msg_type == 2 then 
        subtree:add(buffer(0,1),"DATA_MSG")
        local count = bit.band(bit.rshift(buffer(0,2):uint(),3), 0x1FFF)
        subtree:add(buffer(0,2),"count: " .. count)
        local offset = 2
        for i=0,count-1,1 do
            local id = buffer(offset,2):uint()
            local data = buffer(offset + 2,4):uint()
            subtree:add(buffer(offset,6),"id: " .. id .. " data: " .. data)
            offset = offset + 6
        end
    elseif msg_type == 3 then
        subtree:add(buffer(0,1),"ERR_MSG")
    elseif msg_type == 4 then
        subtree:add(buffer(0,1),"ACK_MSG")
    elseif msg_type == 5 then
        subtree:add(buffer(0,1),"FINIT_MSG")
    elseif msg_type == 6 then
        subtree:add(buffer(0,1),"RECONF_MSG")
    else
        subtree:add(buffer(0,1),"UNKNOWN_MSG " .. msg_type)
    end
end
-- load the udp.port table
udp_table = DissectorTable.get("udp.port")
-- register our protocol to handle udp ports 4000-5000
for i=4000,5000,1 do
    udp_table:add(i,orbisensoriis_proto)
end
