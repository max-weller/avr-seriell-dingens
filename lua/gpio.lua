print("gpio init")
function on_gpio_set(topic, value)
    topic = topic:sub(0, topic:find("/")-1)
    print("on_gpio_set",topic,value,GPIO_ALIASES[topic], GPIO_FLAGS[topic])
    if (value=="true" or value=="1") then
        gpio.write(GPIO_ALIASES[topic], GPIO_FLAGS[topic]~="i" and 1 or 0)
        publish_property(topic..'/on', "true")
    else
        gpio.write(GPIO_ALIASES[topic], GPIO_FLAGS[topic]=="i" and 1 or 0)
        publish_property(topic..'/on', "false")
    end
end
for k,v in pairs(GPIO_ALIASES) do
    properties[k.."/on"] = {set=on_gpio_set, dt="boolean", n=k.." (D"..v..")", v='false'}
    gpio.mode(v, gpio.OUTPUT)
    gpio.write(v, GPIO_FLAGS[k] == "i" and 1 or 0)
end