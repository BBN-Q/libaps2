classdef APS2 < handle
   properties
       serial
       libpath = 'C:\Users\rsl\Downloads\libaps2-v0.2\build';
   end
   
   methods
       function obj = APS2()
           if ~libisloaded('libaps2')
               loadlibrary(fullfile(obj.libpath, 'libaps2.dll'), fullfile(obj.libpath, 'libaps.h'));
               calllib('libaps2', 'init');
           end
       end
       
       function delete(obj)
           unloadlibrary('libaps2')
       end
       
       function connect(obj, serial)
           obj.serial = serial;
           calllib('libaps2', 'connect_APS', serial);
           calllib('libaps2', 'initAPS', serial, 0);
       end
       
       function disconnect(obj)
           calllib('libaps2', 'disconnect_APS', obj.serial);
       end
       
       function run(obj)
           calllib('libaps2', 'run', obj.serial);
       end
       
       function stop(obj)
           calllib('libaps2', 'stop', obj.serial);
       end
       
       function set_trigger_interval(obj, val)
           calllib('libaps2', 'set_trigger_interval', obj.serial, val);
       end
       
       function load_sequence(obj, filename)
           calllib('libaps2', 'load_sequence_file', obj.serial, filename);
       end
   end
    
end