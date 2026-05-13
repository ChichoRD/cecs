#ifndef DCKF_INPUT_H
#define DCKF_INPUT_H

void dckf_reset_terminal_mode(void);
void dckf_set_conio_terminal_mode(void);

int dckf_kbhit(void);
int dckf_getch(void);

#endif
