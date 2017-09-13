include('lib')

sntp.sync()

properties = {}
mqtt_connected_handler = {}

include('ws2812')
include('bmp085')
include('rfswitch')
include('gpio')

uptime=0
local l = file.list();
modules = {}
for k,v in pairs(l) do
    table.insert(modules, k .. "("..v..")")
end
-- mqtt connection
function publish_stats()
    uptime = uptime + STATS_INTERVAL
    pcall(function()
        m:publish(prefix.."$stats/uptime",uptime,0,1)
        m:publish(prefix.."$stats/heap",node.heap(),0,1)
    end)
end
function on_mqtt_connected(client) 
    print("MQTT connected")
    m:publish(prefix.."$online","true",0,1)
    m:publish(prefix.."$localip",wifi.sta.getip(),0,1)
    m:publish(prefix.."$mac",wifi.sta.getmac(),0,1)
    m:publish(prefix.."$name",DEV_NAME,0,1)
    m:publish(prefix.."$stats/interval",STATS_INTERVAL,0,1)
    m:publish(prefix.."$implementation","ham@nodemcu",0,1)
    for k,opts in pairs(properties) do
        if opts.set then
            m:subscribe(prefix..k..'/set',0)
            m:publish(prefix..k..'/$settable', 'true',0,1)
        else
            m:publish(prefix..k..'/$settable', 'false',0,1)
        end
        if opts.u then m:publish(prefix..k..'/$unit', opts.u,0,1) end
        if opts.f then m:publish(prefix..k..'/$format', opts.f,0,1) end
        if opts.v then m:publish(prefix..k, opts.v,0,1) end
        m:publish(prefix..k..'/$datatype', opts.dt,0,1)
        m:publish(prefix..k..'/$name', opts.n,0,1)
    end
end
function publish_property(propname, value)
    properties[propname]['v'] = value
    m:publish(prefix..propname, value, 0, 1)
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
m:lwt(prefix.."$online", "false", 0, 1)
m:on("message",function(client, topic, message)
    print("MQTT:",topic,message)
    topic=topic:sub(prefix:len()+1, topic:len()-4)
    print("MQTT:",topic,message)
    opts = properties[topic]
    if opts ~= nil and opts['set'] ~= nil then
        opts['set'](topic, message)
    end
end)
do_mqtt_connect()

tmr.create():alarm(STATS_INTERVAL, tmr.ALARM_AUTO, publish_stats)

if mdns then mdns.register(string.format("node-%06x",node.chipid()), {port=99,service='secupload',description=(DEV_NAME or "ham")}) end

