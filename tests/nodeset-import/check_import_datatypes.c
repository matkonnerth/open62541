/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"

#include "datatypes.h"

static void setup(void) {

}

static void teardown(void) {

}

START_TEST(StructWithBuiltinTypes) {

    UA_DataType floatStruct;
    floatStruct.membersSize = 1;
    UA_DataTypeMember member;
    member.isArray = false;
    member.memberTypeIndex = UA_TYPES_FLOAT;
    member.namespaceZero = true;
    floatStruct.members = &member;
    setPaddingMemsize(&floatStruct, &UA_TYPES[0], NULL);
    ck_assert_uint_eq(floatStruct.memSize, sizeof(UA_Float));
    ck_assert_uint_eq(member.padding, 0);


    struct Compare
    {
        UA_Boolean b;
        UA_Double d;
    };

    UA_DataType simpleStruct;
    simpleStruct.membersSize = 2;
    UA_DataTypeMember members[2];
    members[0].isArray = false;
    members[0].memberTypeIndex = UA_TYPES_BOOLEAN;
    members[0].namespaceZero = true;
    members[1].isArray = false;
    members[1].memberTypeIndex = UA_TYPES_DOUBLE;
    members[1].namespaceZero = true;
    simpleStruct.members = &members[0];
    setPaddingMemsize(&simpleStruct, &UA_TYPES[0], NULL);
    ck_assert_uint_eq(members[0].padding, 0);
    ck_assert_uint_eq(members[1].padding, offsetof(struct Compare, d) - sizeof(UA_Boolean));
    ck_assert_uint_eq(simpleStruct.memSize, sizeof(struct Compare));
}
END_TEST


static Suite *testSuite_Client(void) {
    Suite *s = suite_create("DataTypes");
    TCase *tc_datatypes = tcase_create("DataTypes");
    tcase_add_unchecked_fixture(tc_datatypes, setup, teardown);
    tcase_add_test(tc_datatypes, StructWithBuiltinTypes);
    suite_add_tcase(s, tc_datatypes);
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
