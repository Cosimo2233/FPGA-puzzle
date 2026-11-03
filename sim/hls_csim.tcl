# 对迁移后的四核源码执行 C 仿真；不综合、不导出 IP、不加载硬件。
set root [file normalize [file dirname [info script]]/..]
foreach {name top folder source test vectors} {
    preprocess puzzle_preprocess {} preprocess.cpp test_preprocess.cpp {}
    rigid puzzle_rigid_fit rigid rigid_fit.cpp test_rigid_fit.cpp rigid_vectors.txt
    clearance puzzle_clearance_scan clearance clearance_scan.cpp test_clearance_scan.cpp clearance_vectors.txt
    candidate puzzle_candidate_scan candidate candidate_scan.cpp test_candidate_scan.cpp candidate_vectors.txt
} {
    set input [file join $root src hls $folder]
    open_project -reset [file join $root build generated csim_$name]
    set_top $top
    add_files [file join $input $source]
    add_files -tb [file join $root sim hls $folder $test] -cflags "-I$input"
    if {$vectors ne ""} {add_files -tb [file join $root data hls $folder $vectors]}
    open_solution -reset solution -flow_target vivado
    set_part xczu5ev-sfvc784-2-i
    create_clock -period 10
    csim_design
    close_project
    puts "PASS RELOCATED CSIM: $name"
}
exit
