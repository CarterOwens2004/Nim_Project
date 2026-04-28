add_executable (Nim 
    "nimNetwork.cpp"
    "nim.cpp"
    "Utilities.cpp"
)
set_property(TARGET Nim PROPERTY CXX_STANDARD 20)
target_link_libraries(Nim ws2_32 iphlpapi)