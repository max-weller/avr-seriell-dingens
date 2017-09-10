
sntp.sync()

-- lights
ws2812.init()
usock = net.createUDPSocket()
usock:listen(1337)
buffer = ws2812.newBuffer(LED_COUNT, 3)

--switch all lights off first
ws2812.write(buffer)
gpio.mode(4,gpio.INPUT)

pkgcount=0
lightfps=0

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
    print(string.format("running with %s fps", lightfps))
    m:publish(prefix.."light/fps",lightfps, 0,0)
    
end)
usock:on("receive", function(s, data, port, ip)
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
function on_light_set(value)
    if fade_timer ~= nil then fade_timer:unregister()  fade_timer = nil end
    udplight_timer:stop()
    if value=="1" then
        buffer:fill(1,1,1)
        fade_timer = alarm_n(55, 8, function()
            buffer:fade(2, ws2812.FADE_IN)
            ws2812.init()
            ws2812.write(buffer)
            gpio.mode(4,gpio.INPUT)
        end)
        m:publish(prefix.."light/state","1",0,0)
    else
        fade_timer = alarm_n(55, 8, function()
            buffer:fade(2)
            ws2812.init()
            ws2812.write(buffer)
            gpio.mode(4,gpio.INPUT)
        end)
        m:publish(prefix.."light/state","0",0,0)
    end
    light_enabled=false
end

-- mqtt connection
function get_info()
    l = file.list();
    modules = ""
    for k,v in pairs(l) do
        modules = modules .. k .. "("..v..") "
    end
    return string.format("Device: %s\nSystem Chip ID: %06x\nIP: %s\nMAC: %s\nSPI Flash ID: %06x\nModules: %s", 
        DEV_NAME, node.chipid(), wifi.sta.getip(), wifi.sta.getmac(), node.flashid(), modules)
end
function on_mqtt_connected(client) 
    print("MQTT connected")
    m:publish(prefix.."status","ONLINE",0,1)
    m:publish(prefix.."info",get_info(),0,1)
    m:subscribe(prefix.."light/set",0)
end
function on_mqtt_error(client, reason) 
    print("MQTT error",reason)
    tmr.create():alarm(10 * 1000, tmr.ALARM_SINGLE, do_mqtt_connect)
end
function do_mqtt_connect()
    print("MQTT setup...")
    m:connect(MQTT_HOST, 1883, 0, on_mqtt_connected, on_mqtt_error)
end

prefix=string.format("ham/%06x/",node.chipid())
m = mqtt.Client(prefix, 60, MQTT_USER, MQTT_PASSWORD)
m:lwt(prefix.."status", "OFFLINE", 0, 1)
m:on("message",function(client, topic, message)
    print("MQTT:",topic,message)
    topic=topic:sub(prefix:len()+1)
    print("MQTT:",topic,message)
    if topic=="light/set" then
        on_light_set(message)
    end
end)
do_mqtt_connect()

include('bmp085')
include('rfswitch')
