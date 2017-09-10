
function include(fn)
    print("Running "..fn.." ...")
    local status, err = pcall(function()
        local p=loadfile(fn..".lua")
        p()
    end)
    if not status then print("ERROR:",err) end
end

include('sysconfig')

-- wifi
wifi.setmode(wifi.STATION)
do
local station_cfg={}
station_cfg.ssid=WIFI_SSID
station_cfg.pwd=WIFI_PSK
wifi.sta.config(station_cfg)
end
wifi.sta.connect()


-- run system services
include('telnet')
include('secupload')


-- wait for connectifity and run application
autostart_timer = tmr.create()
autostart_timer:alarm(1000, 1, function()
   if wifi.sta.getip() == nil then
      print("Connecting to "..WIFI_SSID.."...")
   else
      print('IP: ',wifi.sta.getip())
      autostart_timer:unregister()  autostart_timer=nil
      include(string.format("cfg_%06x",node.chipid()))
      include('application')
   end
end)

