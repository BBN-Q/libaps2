% MATLAB wrapper for the APS2 driver.

% Original author: Blake Johnson
% Date: August 11, 2014

% Copyright 2014 Raytheon BBN Technologies
%
% Licensed under the Apache License, Version 2.0 (the "License");
% you may not use this file except in compliance with the License.
% You may obtain a copy of the License at
%
%     http://www.apache.org/licenses/LICENSE-2.0
%
% Unless required by applicable law or agreed to in writing, software
% distributed under the License is distributed on an "AS IS" BASIS,
% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
% See the License for the specific language governing permissions and
% limitations under the License.

classdef APS2 < handle
    properties
        serial
        libpath = 'C:\Users\qlab\Documents\GitHub\libaps2\build';
    end
    
    methods
        function obj = APS2()
            if ~libisloaded('libaps2')
                curPath = fileparts(mfilename('fullpath'));
                loadlibrary(fullfile(obj.libpath, 'libaps2.dll'), fullfile(curPath, 'libaps.matlab.h'));
            end
        end
        
        function delete(obj)
            try
               obj.disconnect();
            catch %#ok<CTCH>
            end
        end
        
        function [serials] = enumerate(obj)
            numDevices = calllib('libaps2', 'get_numDevices');
            serials = cell(1,numDevices);
            for ct = 1:numDevices
                serials{ct} = '';
            end
            serialPtr = libpointer('stringPtrPtr', serials);
            serials = calllib('libaps2', 'get_deviceSerials', serialPtr);
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
        
        function val = get_running(obj)
            val = calllib('libaps2', 'get_running', obj.serial);
        end
        
        % trigger methods
        function val = get_trigger_interval(obj)
            val = calllib('libaps2', 'get_trigger_interval', obj.serial);
        end
        
        function set_trigger_interval(obj, val)
            calllib('libaps2', 'set_trigger_interval', obj.serial, val);
        end
        
        function val = get_trigger_source(obj)
            triggerSourceMap = containers.Map({0, 1}, {'external', 'internal'});
            val = calllib('libaps2', 'get_trigger_source', obj.serial);
            val = triggerSourceMap(val);
        end
        
        function set_trigger_source(obj, source)
            triggerSourceMap = containers.Map({'external', 'ext', 'internal', 'int'}, {0, 0, 1, 1});
            calllib('libaps2', 'set_trigger_source', obj.serial, triggerSourceMap(lower(source)));
        end
        
        % waveform and instruction data methods
        function load_sequence(obj, filename)
            calllib('libaps2', 'load_sequence_file', obj.serial, filename);
        end

        % channel methods
        function val = get_channel_offset(obj, channel)
            val = calllib('libaps2', 'get_channel_offset', obj.serial, channel-1);
        end
        
        function set_channel_offset(obj, channel, offset)
            calllib('libaps2', 'set_channel_offset', obj.serial, channel-1, offset);
        end

        function val = get_channel_scale(obj, channel)
            val = calllib('libaps2', 'get_channel_scale', obj.serial, channel-1);
        end
        
        function set_channel_scale(obj, channel, scale)
            calllib('libaps2', 'set_channel_scale', obj.serial, channel-1, scale);
        end

        function val = get_channel_enabled(obj, channel)
            val = calllib('libaps2', 'get_channel_enabled', obj.serial, channel-1);
        end
        
        function set_channel_enabled(obj, channel, enabled)
            calllib('libaps2', 'set_channel_enabled', obj.serial, channel-1, enabled);
        end
        
        % debug methods
        function set_logging_level(obj, level)
            calllib('libaps2', 'set_logging_level', level);
        end
    end
    
end