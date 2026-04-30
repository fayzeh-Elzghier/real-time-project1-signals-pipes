#ifndef TEAM_H
#define TEAM_H

#include "config.h"

void test_team_setup(Config config);
void create_team_processes(Config config, int team_id);
void test_pipe_between_two_members(void);
void test_pipe_chain_for_team(Config config, int team_id);
void test_sink_checking(Config config, int team_id);
void test_backward_return(Config config, int team_id);
void test_wrong_then_correct(Config config, int team_id);
void test_source_piece_selection(Config config, int team_id);
void test_selection_with_pipes(Config config, int team_id);
void test_status_pipe(Config config, int team_id);
void test_loop_until_accepted(Config config, int team_id);
void test_accept_all_pieces_in_order(Config config, int team_id);
void test_return_message_protocol(Config config, int team_id);
void test_random_selection_with_blocked_piece(Config config, int team_id);
void random_member_delay(Config config, int moved_count, int team_id, int member_id);
#endif