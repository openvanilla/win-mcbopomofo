add_executable(BopomofoBrailleTests
    test_main.cpp
    test_BopomofoSyllable.cpp
    test_Converter.cpp
)

target_link_libraries(BopomofoBrailleTests PRIVATE BopomofoBraille)
target_include_directories(BopomofoBrailleTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME BopomofoBrailleUnitTests COMMAND BopomofoBrailleTests)
