
add_example(server_import server_import.c)
target_link_libraries(server_import xmlImport)
target_compile_options(server_import PRIVATE -Wall -Wextra -pedantic )