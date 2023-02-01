/*
  - ARRAY (width x height) is initialized to zero
  - Operations
  - Write character to index
  - <NL>
  - <BKSPC>
  - <DEL>
  - Atomic operations
  - Push array forward
  - Pull array back
  - Insert char at index
  - Move index
  - Operation is performed and array evaluated
  - Rules
  - Only allowable chars at row end are <SPC>, <NL>, 0
  - Run algorithm on non-compliant array to bring into compliance
  - First try moving char successively to left
  - If cannot comply then move char successivley to the right
  - When pulling back zeros added from next <NL> or end of array whicever is first
  - Zeroes only allowed consecutively from row end, all others removed

  - Decomposition of operations
  - Write char -> Insert char and push forward 1
  - <NL>       -> Insert NL and position cursor at next row head
  - <DEL>      -> Pull back from current position 
  - <BKSPC>    -> move cursor back 1 pos and perform delete



  - Moving to the left
  - Position index at left-most contiguous zero, count number of steps
  - Pull back that number of steps from end or nearest NL
  - Evaluate for compliance on this row
  - If not compliant push to the right starting at current index
  -Moving to the right
  - Push back and insert 0 at current index
  - Evaluate for compliance on this row
  - repeat
  - chars at last position in array fall off
*/

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define SCN_WIDTH 25
#define SCN_HEIGHT 12
#define KEY_RET 10
#define KEY_SPC 32
#define CTRL_TIL 0

int push_forward(char *editor_str, int size, int index);
int pull_back(char *editor_str, int size, int index);
int count_nulls(char *editor_str, int index);
int count_wrap_chars(char *editor_str, int index);

int input_handler(WINDOW *edit_win, WINDOW *disp_win, char *editor_str, int size);
int handle_KEY_UP(int *cursor_pos);
int handle_KEY_DOWN(int *cursor_pos);
int handle_KEY_LEFT(int *cursor_pos);
int handle_KEY_RIGHT(int *cursor_pos);
int handle_KEY_RET(int *cursor_pos, char *editor_str, int size);
int handle_KEY_DC(int *cursor_pos, char *editor_str, int size);
int handle_KEY_BACKSPACE(int *cursor_pos, char *editor_str, int size);
int handle_char_input(int *cursor_pos, char *editor_str, int size, int ch);

int process_array(char *editor_str, int size, int *cursor_pos);
int try_left_fit(char *editor_str, int size, int index, int *cursor_pos);
int right_fit(char *editor_str, int size, int index, int *cursor_pos);
int print_array(WINDOW *win, char *editor_str);
int print_display(WINDOW *win, char *editor_str);

int main()
{
  initscr();

  WINDOW *edit_win = newwin(SCN_HEIGHT, SCN_WIDTH, 1, 1);
  WINDOW *edit_border_win = newwin(SCN_HEIGHT + 2, SCN_WIDTH + 2, 0, 0);
  WINDOW *disp_win = newwin(SCN_HEIGHT, SCN_WIDTH, 1, SCN_WIDTH + 4);
  WINDOW *disp_border_win = newwin(SCN_HEIGHT + 2, SCN_WIDTH + 2, 0, SCN_WIDTH + 3);  

  cbreak();
  noecho();
  keypad(edit_win, TRUE);
  box(edit_border_win, 0, 0);
  box(disp_border_win, 0, 0);
  refresh();
  wrefresh(edit_win);
  wrefresh(edit_border_win);
  wrefresh(disp_win);
  wrefresh(disp_border_win);

  char editor_str[SCN_WIDTH * SCN_HEIGHT] = {0};
  int editor_str_size = (int)sizeof(editor_str);

  input_handler(edit_win, disp_win, editor_str, editor_str_size);

  endwin();

  return 0;
}

int push_forward(char *editor_str, int size, int index)
{
  for(int i = (size - 1); i > index; i--){
    editor_str[i] = editor_str[i - 1];
  }

  editor_str[index] = 0;

  return 0;
}

int pull_back(char *editor_str, int size, int index)
{
  int i = 0;

  for(i = index; i < (size - 1); i ++){
    if(editor_str[i] == '\n') break;
    editor_str[i] = editor_str[i + 1];
  }

  editor_str[++i] = 0;

  return 0;
}

int process_array(char *editor_str, int size, int *cursor_pos)
{

  for(int i = SCN_WIDTH - 1; i < (size - 1); i += SCN_WIDTH){
    if((editor_str[i] == ' ') || (editor_str[i] == 0) || (editor_str[i] == '\n')) continue;
    if(!try_left_fit(editor_str, size, i, cursor_pos))
      right_fit(editor_str, size, i, cursor_pos);
  }

  return 0;
}

int try_left_fit(char *editor_str, int size, int index, int *cursor_pos)
{
  int result = 0, nulls = 0, last_space = 0;

  nulls = count_nulls(editor_str, index);

  for(int i = 0; i < nulls; i++){
    pull_back(editor_str, size, index - nulls);
    if((editor_str[index] == ' ') || (editor_str[index] == 0) || (editor_str[index] == '\n')){
      last_space = i;
      (*cursor_pos) -= (i + 1);
    }
  }

  if(last_space){
    for(int i = 0; i < (nulls - last_space); i++){
      push_forward(editor_str, size, index - last_space);
    }
    return 1;
  }

  //undo left search
  for(int i = 0; i < nulls; i++){
    push_forward(editor_str, size, index - nulls);
  }
  
  return result;
}

int right_fit(char *editor_str, int size, int index, int *cursor_pos)
{
  int result = 0;
  int wrap_chars = count_wrap_chars(editor_str, index);

  for(int i = 0; i < wrap_chars; i++){
    push_forward(editor_str, size, index - wrap_chars + 1);
  }

  if((*cursor_pos) >= (index - wrap_chars))
    (*cursor_pos) += wrap_chars;

  return result;
}

int count_nulls(char *editor_str, int index){
  int count = 0;

  while(editor_str[index - 1 - count] == 0){
    count++;
    if(count > SCN_WIDTH){
      count--;
      break;
    }
  }
  
  return count;
}

int count_wrap_chars(char *editor_str, int index){
  int count = 1;

  while((editor_str[index - count] != 0) && (editor_str[index - count] != '\n') && (editor_str[index - count] != ' ')){
    count++;
  }

  return count;
}

int input_handler(WINDOW *win, WINDOW *disp_win, char *editor_str, int size)
{
  int ch = 0;
  int cursor_pos = 0;

  while(1){

    wmove(win, cursor_pos / SCN_WIDTH, cursor_pos % SCN_WIDTH);
    wrefresh(win);

    if((ch = wgetch(win)) == CTRL_TIL) break;

    switch(ch){
    case KEY_DOWN:
      handle_KEY_DOWN(&cursor_pos);
      break;
    case KEY_UP:
      handle_KEY_UP(&cursor_pos);
      break;
    case KEY_RIGHT:
      handle_KEY_RIGHT(&cursor_pos);
      break;
    case KEY_LEFT:
      handle_KEY_LEFT(&cursor_pos);
      break;
    case KEY_RET:
      handle_KEY_RET(&cursor_pos, editor_str, size);
      break;
    case KEY_DC:
      handle_KEY_DC(&cursor_pos, editor_str, size);
      break;
    case KEY_BACKSPACE:
      handle_KEY_BACKSPACE(&cursor_pos, editor_str, size);
      break;
    default:
      handle_char_input(&cursor_pos, editor_str, size, ch);
      break;
    }

    process_array(editor_str, size, &cursor_pos);
    print_array(win, editor_str);
    print_display(disp_win, editor_str);
  }

  return 0;
}

int handle_KEY_DOWN(int *cursor_pos){

  (*cursor_pos) += SCN_WIDTH;
  
  return 0;
}

int handle_KEY_UP(int *cursor_pos){

  (*cursor_pos) -= SCN_WIDTH;

  return 0;
}

int handle_KEY_RIGHT(int *cursor_pos){

  (*cursor_pos) += 1;

  return 0;
}

int handle_KEY_LEFT(int *cursor_pos){

  (*cursor_pos) -= 1;

  return 0;
}

int handle_KEY_RET(int *cursor_pos, char *editor_str, int size){
  int shift_count = 0;

  shift_count = (*cursor_pos) % SCN_WIDTH;
  for(int i = 0; i < shift_count; i++){
    push_forward(editor_str, size, *cursor_pos);
  }

  (*cursor_pos) += shift_count;

  return 0;
}

int handle_KEY_DC(int *cursor_pos, char *editor_str, int size){

  pull_back(editor_str, size, *cursor_pos);

  return 0;
}

int handle_KEY_BACKSPACE(int *cursor_pos, char *editor_str, int size){
  int temp_pos = (*cursor_pos);
  
  (*cursor_pos)--;

  while(editor_str[*cursor_pos] == 0)
    (*cursor_pos)--;

  for(int i = 0; i < temp_pos - (*cursor_pos); i ++){
    handle_KEY_DC(cursor_pos, editor_str, size);
  }

  return 0;
}

int handle_char_input(int *cursor_pos, char *editor_str, int size, int ch){

  push_forward(editor_str, size, *cursor_pos);
  editor_str[*cursor_pos] = ch;
  (*cursor_pos)++;

  return 0;
}

int print_array(WINDOW *win, char *editor_str){

  wclear(win);
  wmove(win, 0, 0);

  for(int i = 0; i < SCN_HEIGHT; i++){
    for(int j = 0; j < SCN_WIDTH; j++){
      if((editor_str[i * SCN_WIDTH + j] == '\n') || (editor_str[i * SCN_WIDTH + j] == 0))
	mvwaddch(win, i, j, ' ');
      else mvwaddch(win, i, j, editor_str[i * SCN_WIDTH + j]);
    }
  }

  return 0;
}

int print_display(WINDOW *win, char *editor_str){

  wclear(win);
  wmove(win, 0, 0);

  for(int i = 0; i < SCN_HEIGHT; i++){
    for(int j = 0; j < SCN_WIDTH; j++){
      if(editor_str[i * SCN_WIDTH + j] == 0)
	mvwaddch(win, i, j, '0');
      else if(editor_str[i * SCN_WIDTH + j] == '\n')
	mvwaddch(win, i, j, '%');
      else if(editor_str[i * SCN_WIDTH + j] == ' ')
	mvwaddch(win, i, j, '^');
      else
	mvwaddch(win, i, j, editor_str[i * SCN_WIDTH + j]);
    }
  }

  wrefresh(win);
  
  return 0;
}
