// /std/daemon/daemon.c
// Base daemon inheritible
//
// Created:     2022/08/23: Gesslar
// Last Change: 2022/08/23: Gesslar
//
// 2022/08/23: Gesslar - Created

inherit STD_OBJECT;
inherit M_PERSIST_DATA;

void create(mixed _args...) {
    setup_chain();
}

int is_daemon() {
    return 1;
}
