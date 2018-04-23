-- a secure upload server
if secupload_srv ~= nil then
    secupload_srv:close()
end
secupload_srv = net.createServer(net.TCP, 180)

function buildpacket(nonce,cmd,flags,header_payload)
    local pkg = struct.pack(">BBI2c0", cmd, flags, header_payload:len(), header_payload)
    local hash
    if nonce == nil then hash=string.rep("\0",32) 
    else hash=crypto.hmac("SHA256", nonce .. pkg, SECUPLOAD_KEY) end
    return "su"..hash..pkg
end
function build_discovery_response(nonce)
    local pkg = struct.pack("I4", #properties)
    for k,opts in pairs(properties) do
        pkg = pkg .. struct.pack("BB Bc0 Bc0 Bc0 Bc0 Bc0 ", opts.set and 1 or 0, opts.dt,
            prefix:len()+k:len(), prefix..k, opts.f:len(), opts.f, opts.u:len(), opts.u, opts.n:len(), opts.n, opts.v:len(), opts.v)
    end
    return buildpacket(nonce, 0x81, 0, pkg)
end
function build_publish(nonce, k)
    local pkg = struct.pack("I4", 1)
    opts = properties[k]
    pkg = pkg .. struct.pack("BB Bc0 Bc0 Bc0 Bc0 Bc0 ", opts.set and 1 or 0, opts.dt,
        prefix:len()+k:len(), prefix..k, opts.f:len(), opts.f, opts.u:len(), opts.u, opts.n:len(), opts.n, opts.v:len(), opts.v)
    return buildpacket(nonce, 0x8a, 0, pkg)
end
secupload_srv:listen(99, function(socket)
    if autostart_timer ~= nil then autostart_timer:unregister() autostart_timer = nil end
    HASHLEN = 32
    HEADERLEN=8+HASHLEN
    local fd = nil
    local thehash = nil
    local flags = 0
    local written = 0
    local magic, command, flags, header_hmac, filesize, filehash, filename, data_offset
    local nonce = string.format("%08x%08x", node.random(0,4294967295), node.random(0,4294967295))
    socket:send(buildpacket(nonce, 0x4e, 0, nonce)) -- 'N' nonce
    socket:on("receive", function(c, l)
        local function proto_err(msg)
            socket:send(buildpacket(nonce, 0x46, 0, struct.pack("Bc0", msg:len(), msg))) -- 'E' error
            print("proto_err: "+msg)
            socket:on("sent", function()
                socket:close()
            end)
        end
        if fd then
            fd:write(l)
            written = written + l:len()
            thehash:update(l)
        else
            magic, header_hmac, header_len, command, flags, packet_offset =
                 struct.unpack(">c2 c32 I2 BB ")
            if magic ~= "sc" then proto_err("EMAGIC") return end
            if header_hmac ~= crypto.hmac("SHA256", nonce .. l:sub(35,data_offset-1), SECUPLOAD_KEY) then
                proto_err("AUTH") return end
            if command == 0xe0 then -- file upload
                filesize, filehash, filename, data_offset = struct.unpack(">I4 c32 Bc0", l, packet_offset)
                if file.exists(filename) and crypto.fhash("SHA256", filename) == filehash then
                    proto_err("EQ") return end
                fd = file.open(".tmp", "w")
                if not fd then proto_err("FD") return end
                
                print("Receiving file "..fn.."...")
                c:send(buildpacket(nil,0xef,0,"go"))
                thehash = crypto.new_hash("SHA256")
                thehash:update(nonce)
                data = l:sub(data_offset)
                fd:write(data)
                written = data:len()
            elseif command == 0xe1 then -- file delete
                filesize, filehash, filename, data_offset = struct.unpack(">I4 c32 Bc0", l, packet_offset)
                file.remove(filename)
                c:send(buildpacket(nonce, 0xef, 0, "deleted\n"))
            elseif command == 0xeb then -- reboot
                node.restart()
            elseif command == 0x80 then -- discovery request
                c:send(build_discovery_response(nonce))
            end
        end
        l=nil
        if fd and written >= filesize then
            fd:close()
            local digest = thehash:finalize()
            if digest == headerdigest then
                file.remove(fn)
                file.rename(".tmp", fn)
                c:send(buildpacket(nonce, 0xef, 0, "OK "..written.." "..flags.."\n"))
                if bit.isset(flags, 0) then    -- flag 0x01: run it
                    dofile(fn)
                end
                if bit.isset(flags, 1) then    -- flag 0x02: compile it
                    node.compile(fn)
                end
                if bit.isset(flags, 7) then    -- flag 0x80: delete the file
                    file.remove(fn)
                end
            else
                proto_err("HASH")
            end
            --c:close()
        end
    end)
    socket:on("disconnection", function(c)
        if fd then
            fd:close()
            file.remove(".tmp")
        end
        if bit.isset(flags, 6) then    -- flag 0x40: reboot after upload
            node.restart()
        end
    end)

end)

print("Upload server running...")