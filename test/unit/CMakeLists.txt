add_executable(test_bte test_bte.cpp)
target_link_libraries(test_bte tpie)

add_executable(streaming streaming.cpp)
target_link_libraries(streaming tpie)

add_executable(stream_test stream.cpp)
target_link_libraries(stream_test tpie)


if(NOT WIN32) 
set(BTES ${BTES} ufs mmap)
endif(NOT WIN32)
set(BTES ${BTES} ami_stream cache stdio)

foreach(test source sink sort pull_sort buffer pull_buffer) 
  add_test(streaming_${test} streaming ${test})
endforeach(test)

foreach(bte ${BTES})
  foreach(test basic randomread array)
    add_test(bte_${bte}_${test} test_bte ${bte} ${test})
  endforeach(test)
endforeach(bte)

