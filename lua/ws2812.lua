
-- lights
ws2812.init()
usock = net.createUDPSocket()
usock:listen(1337)
buffer = ws2812.newBuffer(LED_COUNT, 3)

--switch all lights off first
ws2812.write(buffer)
gpio.mode(4,gpio.INPUT)

pkgcount=0

lastcount=0
noaction=0
light_enabled=false
udplight_timer = tmr.create()
udplight_timer:register(5000, 1, function()
    local fps=(pkgcount-lastcount)/5
    if pkgcount==lastcount then noaction = noaction+1 else noaction = 0 end
    if noaction > 10 then
        on_light_set("0")
        udplight_timer:stop()
    end
    lastcount=pkgcount
    print(string.format("running with %s fps", fps))
    m:publish(prefix.."light/fps",fps, 0,0)
end)
usock:on("receive", function(s, data, port, ip)
    if properties["light/on"] == "false" then return end
    if not light_enabled then
        udplight_timer:start()
        ws2812.init()
    end
    pkgcount=pkgcount+1
    --print(string.format("%d. received %d bytes from %s:%d", pkgcount, data:len(), ip, port))
    if data:len() ~= LED_COUNT*3 then return end
    for i = 1, LED_COUNT do
        pos=i*3-2
        r, g, b = data:byte(pos, pos+2)
        --print(i,pos,r,g,b)
        buffer:set(i, g, r, b)
    end
    ws2812.write(buffer)
end)

function alarm_n(interval, count, fn)
    local t = tmr.create()
    local i = 0
    t:alarm(40, tmr.ALARM_AUTO, function()
        fn()
        i=i+1
        if i>10 then t:unregister() end
    end)
    return t
end

function update_light()
    if fade_timer ~= nil then fade_timer:unregister()  fade_timer = nil end
    udplight_timer:stop()
    if properties["light/on"].v == "false" then
        buffer:fill(0,0,0)
    else
        bright = tonumber(properties["light/brightness"]["v"])
        buffer:fill(tonumber(properties["light/blue"]["v"]) * bright,
                    tonumber(properties["light/red"]["v"]) * bright,
                    tonumber(properties["light/green"]["v"]) * bright)
    end
    ws2812.init()
    ws2812.write(buffer)
    gpio.mode(4,gpio.INPUT)
    light_enabled=false
end
function on_light_on_set(topic, value)
    if value=="1" or value=="true" then
        if tonumber(properties["light/brightness"]["v"]) < 0.1 then
            publish_property("light/brightness", "0.2")
        end
        publish_property("light/on","true")
    else
        publish_property("light/on","false")
    end
    update_light()
end
function on_light_color_set(topic, value)
    publish_property(topic, value)
    update_light()
end
properties["light/on"] = {set=on_light_on_set, dt=3, u="", f="", n="LED stripe on", v="false"}
properties["light/brightness"] = {set=on_light_color_set, dt=2, u="", f="0:1", n="LED brightness", v="1"}
properties["light/red"] = {set=on_light_color_set, dt=1, u="", f="0:255", n="Red", v="255"}
properties["light/green"] = {set=on_light_color_set, dt=1, u="", f="0:255", n="Green", v="255"}
properties["light/blue"] = {set=on_light_color_set, dt=1, u="", f="0:255", n="Blue", v="255"}
properties["light/fps"] = {dt=2, u="fps", f="", n="Update rate", v=""}

