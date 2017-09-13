
-- sensor init
i2c.setup(0, 6, 5, i2c.SLOW)
bmp085.setup()

-- timer for reporting
tmr.create():alarm(5000, 1, function()
    local t = bmp085.temperature()
    --print(string.format("Temperature: %s degrees C", t / 10))
    m:publish(prefix.."barometer/temperature",t/10, 0,0)
    
    local p = bmp085.pressure()
    --print(string.format("Pressure: %s mbar", p / 100))
    m:publish(prefix.."barometer/pressure",p, 0,0)

    -- switch off the LED which is on the same pin as SCK
    gpio.write(5,0)
end)
properties["barometer/temperature"] = {dt="float", n="Temperature", u="°C"}
properties["barometer/pressure"] = {dt="integer", n="Pressure", u="Pa"}

