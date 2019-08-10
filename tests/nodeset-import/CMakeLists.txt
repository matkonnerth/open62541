include_directories("${PROJECT_BINARY_DIR}/src_generated/tests")

# Generate types and namespace for ADI
ua_generate_nodeset(
    NAME "tests-testimport"
    NAMESPACE_IDX 2
    OUTPUT_DIR "${GENERATE_OUTPUT_DIR}"
    FILE "${PROJECT_SOURCE_DIR}/tests/nodeset-import/testNodesets/testimport.xml"
    DEPENDS_NS "${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml"
)

add_executable(check_nodeset_import check_nodeset_import.c
                ${UA_NODESET_TESTS_TESTIMPORT_SOURCES}
                ${UA_TYPES_TESTS_TESTIMPORT_SOURCES}
            $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
add_dependencies(check_nodeset_import open62541-generator-ns-tests-testimport)
target_link_libraries(check_nodeset_import ${LIBS})
add_test_valgrind(nodeset_compiler_import ${TESTS_BINARY_DIR}/check_nodeset_import)


get_filename_component(NODESETPATH ${CMAKE_CURRENT_SOURCE_DIR}/testNodesets ABSOLUTE)
target_compile_definitions(check_nodeset_import PRIVATE NODESETPATH="${NODESETPATH}")

add_executable(check_import_datatypes check_import_datatypes.c $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-testplugins>)
target_link_libraries(check_import_datatypes ${LIBS})

target_include_directories(check_import_datatypes PRIVATE ${CMAKE_SOURCE_DIR}/plugins/xmlimport)