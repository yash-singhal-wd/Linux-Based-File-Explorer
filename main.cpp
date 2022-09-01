#include "myheader.h"

using namespace std;

void display_arr_on_terminal( int current_cursor_pos, vector <string> &arr);
string get_parent_directory(string path);
void open_file(string path);
vector<string> record_keeper;

/**** Initial terminal attributes and related functions ****/
struct terminal_configs {
    int row_no;
    int number_of_rows_terminal;
    int number_of_cols_terminal;
    int start_row;
    int end_row;
    int window_size;
    string current_path;
    struct termios orig_termios;
    stack<string> prev_stack;
    stack<string> next_stack;
};
struct terminal_configs E;

int get_terminal_rows_and_cols(int *rows, int *cols){
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
      return -1;
    } else {
      *cols = ws.ws_col;
      *rows = ws.ws_row;
      return 0;
    }
}

void initialise_terminal(){
  if( get_terminal_rows_and_cols(&E.number_of_rows_terminal, &E.number_of_cols_terminal) == -1)
    die("get_terminal_rows_and_cols");
  E.row_no = 0;
  E.window_size = E.number_of_rows_terminal-5;
  E.start_row=0;
  E.end_row=0;
  E.current_path="/home/yash";
}

/** helper functions **/
bool is_directory(string path){
    struct stat file_data;
    const char* temp_path = path.c_str();
    stat(temp_path, &file_data);
    string is_dir="";
    is_dir += ((S_ISDIR(file_data.st_mode))  ? "d" : "-");
    if( is_dir=="-" ) return false;
    return true;
}

string get_parent_directory(string path){
    int p2 = path.length();
    int i;
    for(i=p2-1; path[i]!='/'; --i);
    string parent = path.substr(0, i);
    if(i==0) parent = "/"; 
    return parent; 
}

void open_file(string path){
    pid_t child=fork();
        if(child==0){
            string open_path_name;
            if(E.current_path=="/") open_path_name = "xdg-open " + E.current_path + "\'" + path + "\'"; 
            else open_path_name = "xdg-open " + E.current_path + "/" + "\'" + path + "\'";
            execl("/bin/sh", "sh", "-c", open_path_name.c_str() , (char *) NULL);
        }
}

void print_normal_mode_at_end(){
    gotoxy(0, E.number_of_rows_terminal-1);
    cout << "---------------NORMAL MODE--------------- : " << E.current_path << endl;
    gotoxy  (0,0); 
}
/**** Error handling funtion ****/
void die(const char *s) {
    // render_blank_screen();
    // reposition_cursor_to_start();
    perror(s);
    exit(1);
}


/**** Modes ****/
void enable_canonical_mode() {
  if ( tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios)==-1 ) 
      die("tcsetattr");
}

void enable_non_canonical_mode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios)==-1)
        die("tcsgetattr");
    atexit(enable_canonical_mode);
    struct termios raw = E.orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)==-1) die("tcssetattr");
}


/**** cursor related ****/
void reposition_cursor_to_start(){
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void gotoxy(int x, int y){
    printf("%c[%d;%df",0x1B,y,x);
}


/**** output screen related ****/
void render_blank_screen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
}


int get_files(const char* pathname){
    record_keeper.clear();
    E.end_row=0;
    DIR* dir = opendir(pathname);
    if( dir==NULL ){
        render_blank_screen();
        cout << "No such directory!" << endl;
        return 1;
    }

    struct dirent* entity;
    entity = readdir(dir);
    while(entity!=NULL){
        record_keeper.push_back(entity->d_name);
        entity = readdir(dir);
    }
    sort(record_keeper.begin(), record_keeper.end());

    display_arr_on_terminal(E.row_no, record_keeper);
    reposition_cursor_to_start();
    closedir(dir);
    return 0;
}

void display_arr_on_terminal(int current_cursor_pos, vector<string> &arr){
    render_blank_screen();
    string cursor = "       ";
    
    if(E.row_no>(E.end_row-1)){
        E.start_row = E.end_row;
        E.end_row = arr.size()>(E.end_row+E.window_size) ? (E.end_row+E.window_size) : arr.size();
    } else {
        if(E.row_no<E.start_row && E.row_no!=0 && E.start_row!=0){
            E.end_row = E.start_row;
            E.start_row = (E.start_row-E.window_size)>0 ? (E.start_row-E.window_size) : 0;
        }
    }
    for( int i=E.start_row; i<E.end_row; ++i ){
        if( i==current_cursor_pos ) cursor=">>>  "; 
        else cursor = "     ";
        struct stat file_data;
        string temp = E.current_path;
        if(i==0) temp=E.current_path;
        else if(i==1){
            if( !E.prev_stack.empty()) temp=E.prev_stack.top();
            else temp=E.current_path;
        } 
        else temp = temp + "/" + record_keeper[i];
        
        const char* temp_path = temp.c_str();
        stat(temp_path, &file_data);  

        /*** modified time ***/
        string modified_time = ctime(&file_data.st_mtime);
        modified_time = modified_time.substr(4, 20);
        /*** filesize ***/
        string size_of_file="";
        int file_size = file_data.st_size;
        if( file_size>=1024 ){
            file_size = file_size/1024;
            if( file_size>=1024 ){
                file_size = file_size/1024;
                size_of_file = to_string(file_size);
                size_of_file = size_of_file + "GB";
            } else {
                size_of_file = to_string(file_size);
                size_of_file = size_of_file + "KB";
            }
        } else if(file_size>0) {
            size_of_file = to_string(file_size);
            size_of_file = size_of_file + "B";
        } else {
            size_of_file = to_string(0);
            size_of_file = size_of_file + "B";
        }

        /*** permissions ***/
        string permissions="";
        permissions += ((S_ISDIR(file_data.st_mode))  ? "d" : "-");
        permissions += ((file_data.st_mode & S_IRUSR) ? "r" : "-");
        permissions += ((file_data.st_mode & S_IWUSR) ? "w" : "-");
        permissions += ((file_data.st_mode & S_IXUSR) ? "x" : "-");
        permissions += ((file_data.st_mode & S_IRGRP) ? "r" : "-");
        permissions += ((file_data.st_mode & S_IWGRP) ? "w" : "-");
        permissions += ((file_data.st_mode & S_IXGRP) ? "x" : "-");
        permissions += ((file_data.st_mode & S_IROTH) ? "r" : "-");
        permissions += ((file_data.st_mode & S_IWOTH) ? "w" : "-");
        permissions += ((file_data.st_mode & S_IXOTH) ? "x" : "-");

        /*** User and group name ***/
        uid_t user_id = file_data.st_uid;
        uid_t group_id = file_data.st_gid;
        string username = (getpwuid(user_id)->pw_name);
        string groupname = (getgrgid(group_id)->gr_name);

        cout << cursor << permissions << "\t\t" << username << "\t\t" << groupname << "\t\t" << modified_time << "\t\t" << size_of_file << "\t\t" << arr[i] << endl;        
    }
}


int main() {
    enable_non_canonical_mode();
    render_blank_screen();
    initialise_terminal();
    reposition_cursor_to_start();
    const char * the_path = E.current_path.c_str();
    get_files(the_path);
    print_normal_mode_at_end();
    char c;
    while(1){
        c = cin.get();
        if( c==10 /*enter key*/ ){
            string sub_path = record_keeper[E.row_no];
            E.prev_stack.push(E.current_path);
            string to_check_status_dir = E.current_path + "/" + sub_path;
            if(E.current_path=="/") to_check_status_dir=E.current_path+sub_path;
            if(is_directory(to_check_status_dir)){
                if(E.current_path=="/") E.current_path=E.current_path+sub_path;
                else E.current_path = E.current_path + "/" + sub_path;
                E.start_row=0;
                E.end_row=E.window_size-1;
                E.row_no=0;
                the_path = E.current_path.c_str();
                get_files(the_path);
                print_normal_mode_at_end();
            } else {
                open_file(sub_path);
                print_normal_mode_at_end();
            }
        } else if( c=='h' ){
            if( E.current_path!="/home/yash"){
                string home_path = "/home/yash";
                E.prev_stack.push(E.current_path);
                E.current_path = home_path;
                E.start_row=0;
                E.end_row=E.window_size-1;
                E.row_no=0;
                the_path = E.current_path.c_str();
                get_files(the_path);
                print_normal_mode_at_end();
            }
        } else if( c==127 /*backspace*/ ){
            if(E.current_path!="/"){
                string parent = get_parent_directory(E.current_path);
                E.prev_stack.push(E.current_path);
                E.current_path = parent;
                E.start_row=0;
                E.end_row=E.window_size-1;
                E.row_no=0;
                the_path = E.current_path.c_str();
                get_files(the_path);
                print_normal_mode_at_end();
            }
        } 
        else if( c=='q' ) {
            render_blank_screen();
            reposition_cursor_to_start();
            break;
        } else if (c == 'A' /*up*/ ) {
            if(E.row_no>0) E.row_no--;
            display_arr_on_terminal(E.row_no, record_keeper);
            reposition_cursor_to_start();
            print_normal_mode_at_end();
        } else if(c == 'B' /*down*/){
            if( E.row_no < record_keeper.size()-1 ) E.row_no++;
            display_arr_on_terminal(E.row_no, record_keeper);
            reposition_cursor_to_start();
            print_normal_mode_at_end();
        } else if(c == 'C' /*right*/){
            if( !E.next_stack.empty() ){
                E.prev_stack.push(E.current_path);
                E.current_path = E.next_stack.top();
                E.next_stack.pop();
                E.start_row=0;
                E.end_row=E.window_size-1;
                E.row_no=0;
                the_path = E.current_path.c_str();
                get_files(the_path);
                print_normal_mode_at_end();
            }
        } else if(c == 'D' /*left*/){
            if( !E.prev_stack.empty() ){
                E.next_stack.push(E.current_path);
                E.current_path = E.prev_stack.top();
                E.prev_stack.pop();
                the_path = E.current_path.c_str();
                E.start_row=0;
                E.end_row=E.window_size-1;
                E.row_no=0;
                get_files(the_path);
                print_normal_mode_at_end();

            }
        } else if( c == 'p' ){
            print_normal_mode_at_end();
        }
    }
  return 0;
}