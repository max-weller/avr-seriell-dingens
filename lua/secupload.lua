-- a secure upload server
if secupload_srv ~= nil then
    secupload_srv:close()
end
secupload_srv = net.createServer(net.TCP, 180)

secupload_srv:listen(99, function(socket)
    if autostart_timer ~= nil then autostart_timer:unregister() autostart_timer = nil end
    HASHLEN = 32
    HEADERLEN=8+HASHLEN
    local fd = nil
    local fn = nil
    local thehash = nil
    local flags = 0
    local written = 0
    local headerdigest = nil
    local filesize = 0
    local nonce = string.format("%08x%08x", node.random(0,4294967295), node.random(0,4294967295))
    socket:send("SU_NONCE "..nonce.."\n")

    socket:on("receive", function(c, l)
        if fd then
            fd:write(l)
            written = written + l:len()
            thehash:update(l)
        else
            if l:sub(1,2) ~= "SU" then
                c:send("MAGIC\n")
                c:on("sent", function()
                    c:close()
                end)
                return
            end
            local fnlen = l:byte(3)
            local filesize_h, filesize_l = l:byte(7,8)
            filesize = filesize_l + filesize_h*256
            headerdigest = l:sub(9,9+HASHLEN-1)
            flags = l:byte(4)
            fn = l:sub(HEADERLEN+1, HEADERLEN+fnlen+1)
            fd = file.open(".tmp", "w")
            if fd then
                print("Receiving file "..fn.."...")
                c:send("ACK\n")
                thehash = crypto.new_hmac("SHA256", SECUPLOAD_KEY)
                thehash:update(nonce)
                thehash:update(l:sub(1,HEADERLEN-HASHLEN))
                thehash:update(fn)
            else
                print("Invalid file "..fn)
                c:send("ERR\n")
            end
            data = l:sub(fnlen+HEADERLEN+1)
            fd:write(data)
            written = written + l:len() - fnlen - HEADERLEN
            thehash:update(data)
        end
        l=nil
        if written >= filesize then
            fd:close()
            local digest = thehash:finalize()
            if digest == headerdigest then
                file.remove(fn)
                file.rename(".tmp", fn)
                c:send("OK "..written.." "..flags.."\n")
                if bit.isset(flags, 0) then    -- flag 0x01: run it
                    dofile(fn)
                end
                if bit.isset(flags, 7) then    -- flag 0x80: delete the file
                    file.remove(fn)
                end
            else
                c:send("MISMATCH\n")
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