function aps2_demo(addr)

aps = APS2();
connect(aps, addr);

[ver, ver_str, git_sha1, build_timestamp] = get_firmware_version(aps);

fprintf('Connected to APS2 running firmware version %s and up %f seconds\n', ver_str, get_uptime(aps))

stop(aps);
init(aps);

set_trigger_interval(aps, 0.02);
set_trigger_source(aps, 'internal');

fprintf('Playing square waveform\n');
wf = [zeros(100,1); ones(200,1); zeros(100,1)];
load_waveform(aps, 1, wf);
load_waveform(aps, 2, wf);
set_run_mode(aps, 'TRIG_WAVEFORM');
run(aps);
input('Press ANY key to continue: ')
stop(aps);

fprintf('Playing ramp waveforms\n');
wf = linspace(-1,1,2^14);
load_waveform(aps, 1, wf);
load_waveform(aps, 2, -wf);
run(aps);
input('Press ANY key to continue: ')
stop(aps);

fprintf('Playing cos/sin waveform\n');
load_waveform(aps, 1, ones(1200))
load_waveform(aps, 2, zeros(1200))
set_waveform_frequency(aps, 10e6)
set_run_mode(aps, 'CW_WAVEFORM');
run(aps);
input('Playing ANY key to continue: ')
stop(aps);

[example_path,~,~] = fileparts(mfilename('fullpath'));
aps.set_run_mode('RUN_SEQUENCE')

function demo_sequence(display_text, sequence_file)
    fprintf(display_text);
    load_sequence(aps, fullfile(example_path, [sequence_file, '.h5']));
    run(aps);
    input('\nPress ANY key to continue: ')
    stop(aps);
end

demo_sequence('Playing Ramsey with slipped markers', 'ramsey_slipped')

demo_sequence('Playing TPPI Ramsey sequence (modulation instructions)', 'ramsey_tppi');

demo_sequence('Playing TPPI Ramsey sequence with SSB (modulation instructions)', 'ramsey_tppi_ssb');

demo_sequence('Playing Rabi amplitude sweep (waveform prefetching)', 'rabi_amp');

demo_sequence('Playing CPMG (repeats and subroutines)', 'cpmg');

demo_sequence('Playing flip-flops (instruction prefetching)', 'instr_prefetch');

disconnect(aps);

end
