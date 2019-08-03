/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "testing_clock.h"
#include "src_generated/open62541/namespace_tests_testimport_generated.h"
#include "unistd.h"

#include <open62541/plugin/nodesetLoader.h>

#define CAT(a,b) a #b

#define TESTIMPORTXML NODESETPATH "/testimport.xml"
#define BASICNODECLASSTESTXML NODESETPATH "/basicNodeClassTest.xml"

UA_Server *server;

static void setup(void) {
    printf("path to testnodesets %s\n", NODESETPATH);
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_addTestNodeset) {
    UA_StatusCode retval = namespace_tests_testimport_generated(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_ImportNodeset) {
    FileHandler f;
	f.addNamespace = UA_Server_addNamespace;
    f.userContext = server;
    f.file = TESTIMPORTXML;
    UA_StatusCode retval = UA_XmlImport_loadFile(&f);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_ImportNoFile) {
    FileHandler f;
	f.addNamespace = UA_Server_addNamespace;
    f.userContext = server;
    f.file = "notExistingFile.xml";
    UA_StatusCode retval = UA_XmlImport_loadFile(&f);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
}
END_TEST

START_TEST(Server_EmptyHandler) {
    UA_StatusCode retval = UA_XmlImport_loadFile(NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
}
END_TEST

START_TEST(Server_ImportBasicNodeClassTest) {
    FileHandler f;
	f.addNamespace = UA_Server_addNamespace;
    f.userContext = server;
    f.file = BASICNODECLASSTESTXML;
    UA_StatusCode retval = UA_XmlImport_loadFile(&f);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Import");
    TCase *tc_server = tcase_create("Server Import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_addTestNodeset);
    tcase_add_test(tc_server, Server_ImportNodeset);
    tcase_add_test(tc_server, Server_ImportNoFile);
    tcase_add_test(tc_server, Server_EmptyHandler);
    tcase_add_test(tc_server, Server_ImportBasicNodeClassTest);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char*argv[]) {
    printf("%s", argv[0]);
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
