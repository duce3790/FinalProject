#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#define Read_Write
#define GAME_TERMINATE -1
#define PLAY_AGAIN 69
#define MAX_BULLET 14
#define MAX_BULLET_ENEMY 20
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define SPACE 4
#define B 5

// ALLEGRO Variables
ALLEGRO_DISPLAY* display = NULL;
ALLEGRO_EVENT_QUEUE *event_queue = NULL;
ALLEGRO_BITMAP *image = NULL;
ALLEGRO_BITMAP *image2 = NULL;
ALLEGRO_BITMAP *image3 = NULL;
ALLEGRO_BITMAP *background = NULL;
ALLEGRO_KEYBOARD_STATE keyState ;
ALLEGRO_TIMER *timer = NULL;
ALLEGRO_TIMER *timer2 = NULL;
ALLEGRO_TIMER *timer3 = NULL;
ALLEGRO_SAMPLE *song=NULL;
ALLEGRO_SAMPLE *song_AGAIN = NULL;
ALLEGRO_FONT *font = NULL;
ALLEGRO_BITMAP *load_bitmap_at_size(const char *filename, int w, int h);
ALLEGRO_DISPLAY_MODE disp_data;
void game_abort(const char* format, ...);
void game_log(const char* format, ...);
void game_vlog(const char* format, va_list arg);

//Custom Definition
const char *title = "Final Project 107060007";
const float FPS = 60;
const int WIDTH = 1750;//disp_data.width
const int HEIGHT = 800;
float MAX_COOLDOWN = 0.1;
float enemy_MAX_COOLDOWN = 0.6;
float item_MAX_COOLDOWN = 5;
double last_shoot_timestamp;
double enemy_shoot_timestamp;
//draw player blood
float blood_top_x = 5.0;
float blood_top_y = 10.0;
float blood_height = 20.0;
float blood_width = 300.0;
float blood_between_distance = 1470.0;
float blood_down_x;
float blood_down_y;
float blood_down_temp;
//draw player blood
float enemy_blood_down_x;
float blood_top_temp;
//set injury
float injury = 0.0;
float enemy_injury = 0.0;
typedef struct character
{
    int h;
    int w;
    float x,y;
    int vx,vy;
    bool hidden;
    ALLEGRO_BITMAP *image_path;
    int probability;

}Character;

Character character1;
Character character2;
Character character3;
character bullets[MAX_BULLET];
character enemy_bullets[MAX_BULLET_ENEMY];
Character special_item_speed[2];
Character special_item_injury[2];
Character special_item_bullet;
Character special_item_score[10];
Character special_item_blood[3];
Character special_item_star;

int imageWidth = 0;
int imageHeight = 0;
int draw = 0;
int done = 0;
int window = 1;
bool judge_next_window = false;
bool ture = true; //true: appear, false: disappear
bool next = false; //true: trigger
bool dir = true; //true: left, false: right
int score = 0;
bool *keys;
bool *mouse_state;
bool input_file = false;
bool out_file = false;
int injury_time = 0;
bool is_injury = true;
bool store = false;
bool is_death_anime = false;  // 偵測是否跑過死亡動畫
//following code is to define the item we buy from shop
int bullet_addtional_v = 0;  // let players bullet faster
int enemy_addtional_v = 0;   // let enemy slower
bool buy_add_bullet = false;
int bullet_addtional_injury = 0;

void show_err_msg(int msg);
void game_init();
void game_begin();
int process_event();
int game_run();
void game_destroy();
void death_animation(Character);
void reflect(int, int, int, int, int);
void play_music();
void input();
void output();
void item_initialize();
void update_item();
void eat_item();
void draw_item();

int main(int argc, char *argv[]) {
    #ifdef Read_Write
    freopen ("file.in", "r", stdin);
    #endif
    int msg = 0;

    srand(time(NULL));

    game_init();
    game_begin();

    while (msg != GAME_TERMINATE) {
        msg = game_run();
        if (msg == GAME_TERMINATE)
            printf("Game Over\n");
        else if(msg == PLAY_AGAIN){
            al_destroy_sample(song);
            play_music();
            game_begin();
        }
    }
    #ifdef Read_Write
    freopen ("file.in", "w", stdout);
    #endif
    if(out_file) output();
    game_destroy();
    return 0;
}

void show_err_msg(int msg) {
    fprintf(stderr, "unexpected msg: %d\n", msg);
    game_destroy();
    exit(9);
}

void game_init() {
    if (!al_init()) {
        show_err_msg(-1);
    }
    if(!al_install_audio()){
        fprintf(stderr, "failed to initialize audio!\n");
        show_err_msg(-1);
    }
    if(!al_init_acodec_addon()){
        fprintf(stderr, "failed to initialize audio codecs!\n");
        show_err_msg(-1);
    }
    if (!al_reserve_samples(1)){
        fprintf(stderr, "failed to reserve samples!\n");
        show_err_msg(-1);
    }
    // Create display
    display = al_create_display(WIDTH, HEIGHT);
    event_queue = al_create_event_queue();
    if (display == NULL || event_queue == NULL) {
        show_err_msg(-1);
    }
    // Initialize Allegro settings
    keys = (bool*) malloc(80 * sizeof(bool));
    memset(keys, false, 80*sizeof(bool));
    al_get_display_mode(al_get_num_display_modes() - 1, &disp_data);
    al_set_window_position(display, 0, 0);
    al_set_window_title(display, title);
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_audio();
    al_init_image_addon();
    al_init_acodec_addon();
    al_init_font_addon();
    al_init_ttf_addon();

    // Register event
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
}

void game_begin() {
    // Load sound
    song = al_load_sample( "hello.wav" );
    if (!song){
        printf( "Audio clip sample not loaded!\n" );
        show_err_msg(-1);
    }
    // Loop the song until the display closes
    al_play_sample(song, 1.0, 0.0,1.0,ALLEGRO_PLAYMODE_LOOP,NULL);
    // Load and draw text
    font = al_load_ttf_font("pirulen.ttf",70,0);
    al_draw_bitmap(load_bitmap_at_size("menu.png",WIDTH,HEIGHT),0,0,0);
    al_draw_textf(font,al_map_rgb(0,0,255),WIDTH / 3, 0, 0,"score : %d", score);
    al_flip_display();
}

int process_event(){
    // Request the event
    ALLEGRO_EVENT event;
    al_wait_for_event(event_queue, &event);

    if(!store){
        // Our setting for controlling animation
        if(event.timer.source == timer){
            if(character2.y < 0)
                dir = false;
            else if(character2.y > HEIGHT - character2.h)
                dir = true;

            character2.vy = abs(rand()%10 + enemy_addtional_v);

            if(dir) character2.y -= character2.vy ;
            else character2.y += character2.vy;
            character3.x = character2.x;
            character3.y = character2.y;
        }
        if(event.timer.source == timer2){
            ture = false;
            next = true;
        }
        if(event.timer.source == timer3){
            if(next) next = false;
            else ture = true;
        }
        // Keyboard
        //
        if(event.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch(event.keyboard.keycode) {
                case ALLEGRO_KEY_UP:
                    keys[UP] = true;
                    break;
                case ALLEGRO_KEY_DOWN:
                    keys[DOWN] = true;
                    break;
                case ALLEGRO_KEY_RIGHT:
                    keys[RIGHT] = true;
                    break;
                case ALLEGRO_KEY_LEFT:
                    keys[LEFT] = true;
                    break;
                case ALLEGRO_KEY_SPACE:
                    keys[SPACE] = true;
                    break;
                // For Start
                case ALLEGRO_KEY_ENTER:
                    judge_next_window = true;
                    break;
                case ALLEGRO_KEY_B:
                    //judge_next_window = true;  // 不太懂
                    store = true;
                    break;
                case ALLEGRO_KEY_ALT:    // setting
                    printf("will enter setting\n");
                    window = 6;
                    al_destroy_sample(song);
                    printf("will play new song\n");
                    song = al_load_sample("01-What MakesYou Beautiful.flac");
                    printf("play succeed\n");
                    al_play_sample(song, 1.0, 0.0, 1.0,ALLEGRO_PLAYMODE_LOOP,NULL);
                    break;
            }
        }
        else if(event.type == ALLEGRO_EVENT_KEY_UP) {
            switch(event.keyboard.keycode) {
                case ALLEGRO_KEY_UP:
                    keys[UP] = false;
                    break;
                case ALLEGRO_KEY_DOWN:
                    keys[DOWN] = false;
                    break;
                case ALLEGRO_KEY_RIGHT:
                    keys[RIGHT] = false;
                    break;
                case ALLEGRO_KEY_LEFT:
                    keys[LEFT] = false;
                    break;
                case ALLEGRO_KEY_SPACE:
                    keys[SPACE] = false;
                    break;
                case ALLEGRO_KEY_ESCAPE:
                    return GAME_TERMINATE;
                    break;
            }
        }

        character1.y -= keys[UP] * character1.vy;
        character1.y += keys[DOWN] * character1.vy;
        character1.x -= keys[LEFT] * character1.vx;
        character1.x += keys[RIGHT] * character1.vx;

        // TODO: Limit the plane's collision box inside the frame.
        if (character1.x + character1.w / 2 < 0)
            character1.x = 0;
        else if (character1.x - character1.w / 2 > WIDTH)
            character1.x = WIDTH - character1.w / 2;
        if (character1.y +  character2.h / 2 < 0)
            character1.y = 0;
        else if (character1.y + character1.h / 2 > HEIGHT)
            character1.y = HEIGHT - character1.h / 2;

        // TODO: Update bullet coordinates.
        int i;
        for (i = 0; i < MAX_BULLET; i += 2) {
            if (bullets[i].hidden)
                continue;
            bullets[i].x += bullets[i].vx;
            bullets[i].y += bullets[i].vy;
            if(buy_add_bullet){
                bullets[i+1].x += bullets[i].vx;
                bullets[i+1].y += bullets[i].vy;
            }
            if (bullets[i].x + bullets[i].w > WIDTH ){
                bullets[i].hidden = true;
                bullets[i+1].hidden = true;
            }
        }

        for (i = 0; i < MAX_BULLET_ENEMY; i += 2) {
            if (enemy_bullets[i].hidden)
                continue;
            enemy_bullets[i].x += enemy_bullets[i].vx;
            enemy_bullets[i].y += enemy_bullets[i].vy;
            enemy_bullets[i+1].x += enemy_bullets[i+1].vx;
            enemy_bullets[i+1].y += enemy_bullets[i+1].vy;
            if (enemy_bullets[i].x < 0){
                enemy_bullets[i].hidden = true;
                enemy_bullets[i+1].hidden = true;
            }
        }

        // TODO: Shoot if key is down and cool-down is over.
        double now = al_get_time();
        if (keys[SPACE] && (now - last_shoot_timestamp) >= MAX_COOLDOWN) {
            for (i = 0; i < MAX_BULLET; i +=2) {
                if (bullets[i].hidden)
                    break;
            }
            if (i != MAX_BULLET){
                last_shoot_timestamp = now;
                bullets[i].hidden = false;
                bullets[i].x = character1.x + character1.w;
                bullets[i].y = character1.y + character1.h  / 3;
                if(buy_add_bullet){
                    bullets[i+1].hidden = false;
                    bullets[i+1].x = character1.x + character1.w;
                    bullets[i+1].y = character1.y + character1.h * 2 / 3;
                }

            }
        }

        now = al_get_time();
        if(now - enemy_shoot_timestamp >= enemy_MAX_COOLDOWN){
            for (i = 0; i < MAX_BULLET_ENEMY; i += 2) {
                if (enemy_bullets[i].hidden)
                    break;
            }
            if (i != MAX_BULLET_ENEMY){
                enemy_shoot_timestamp = now;
                enemy_bullets[i].hidden = false;
                enemy_bullets[i+1].hidden = false;
                if(dir){
                    enemy_bullets[i].x = character2.x;
                    enemy_bullets[i].y = character2.y - character2.h / 3;
                    enemy_bullets[i+1].x = character2.x;
                    enemy_bullets[i+1].y = character2.y - character2.h * 2 / 3;
                }
                else{
                    enemy_bullets[i].x = character3.x;
                    enemy_bullets[i].y = character3.y - character3.h / 2;
                    enemy_bullets[i+1].x = character2.x;
                    enemy_bullets[i+1].y = character2.y - character2.h * 2 / 3;

                }
            }
        }

        //plane injury
        for( i = 0; i < MAX_BULLET_ENEMY; ++i){
            if((enemy_bullets[i].x > character1.x) && (enemy_bullets[i].x < character1.x + character1.w)
               &&(enemy_bullets[i].y > character1.y) && (enemy_bullets[i].y < character1.y + character1.h)){
                injury = 5.0;
                enemy_bullets[i].hidden = true;
                break;
            }
        }

        // enemy injury
        for( i = 0; i < MAX_BULLET; ++i){
            if((bullets[i].x > character2.x) && (bullets[i].x < character2.x + character2.w)
               &&(bullets[i].y > character2.y) && (bullets[i].y < character2.y + character2.h)){
                enemy_injury = 1.0;
                enemy_injury += bullet_addtional_injury;
                bullets[i].hidden = true;
                break;
            }
        }

        //  一些特殊道具
        for(i = 0; i < 2; ++i) special_item_speed[i].probability = (rand() % 500000) % 500000;
        for(i = 0; i < 2; ++i) special_item_injury[i].probability = (rand() % 500000) % 500000;
        //special_item_bullet.probability = (rand() % 100) % 2147483;
        for(i = 0; i < 10; ++i) special_item_score[i].probability = (rand() % 500000) % 1000;
        for(i = 0; i < 3; ++i) special_item_blood[i].probability = (rand() % 500000) % 10000;
        //special_item_star.probability = (rand() % 100) % 2147483;

        for(i = 0; i < 2; ++i){
            if(special_item_speed[i].probability == 0)
                special_item_speed[i].hidden = false;
            if(special_item_injury[i].probability == 0)
                special_item_injury[i].hidden = false;

        }
            //if(special_item_bullet[i].probability == 0)
                //special_item_bullet[i].hidden = false;
        for(i = 0; i < 10; ++i){
            if(special_item_score[i].probability == 0)
                special_item_score[i].hidden = false;
        }
        for(i = 0; i < 3; ++i){
            if(special_item_blood[i].probability == 0)
                special_item_blood[i].hidden = false;
        }
            //if(special_item_star[i].probability == 0)
            //    special_item_star[i].hidden = false;

        update_item();
        eat_item();
    }

    // Shutdown our program
    if(event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        return GAME_TERMINATE;

    return 0;
}

int game_run(){
    int error = 0;
    // First window(Menu)
    if(window == 1){
        if (!al_is_event_queue_empty(event_queue)) {
            error = process_event();
            if(judge_next_window) {
                window = 2;
                // Setting Character
                character1.w = 200;
                character1.h = 150;
                character1.x = WIDTH / 2;
                character1.y = HEIGHT / 2 + 150;
                character1.vx = (character1.w / 6);
                character1.vy = (character1.h / 6);
                character2.w = 100;
                character2.h = 100;
                //character2.x = WIDTH + 100;
                //character2.y = HEIGHT / 2 - 280;
                character2.x = WIDTH - 2 * character2.w;
                character2.y = HEIGHT - 2 * character2.h;
                character2.vx = 0;
                character2.vy = 20;
                character1.image_path = load_bitmap_at_size("pegasus.png",character1.w,character1.h);
                character2.image_path = load_bitmap_at_size("teemo_left.png",character2.w,character2.h);
                character3.image_path = load_bitmap_at_size("teemo_right.png",character2.w,character2.h);
                background = load_bitmap_at_size("background_2.jpg",WIDTH,HEIGHT);
                int i;
                for (i = 0; i < MAX_BULLET; i++) {
                    bullets[i].w = 60;
                    bullets[i].h = 30;
                    bullets[i].image_path = load_bitmap_at_size("bullet.png",bullets[i].w,bullets[i].h);
                    bullets[i].vx = 20 + bullet_addtional_v;
                    bullets[i].vy = 0;
                    bullets[i].hidden = true;
                }

                for (i = 0; i < MAX_BULLET_ENEMY; i++) {
                    enemy_bullets[i].w = 30;
                    enemy_bullets[i].h = 30;
                    enemy_bullets[i].image_path = load_bitmap_at_size("bulletbt.png",enemy_bullets[i].w,enemy_bullets[i].h);
                    enemy_bullets[i].vx = -20;
                    enemy_bullets[i].vy = 0;
                    enemy_bullets[i].hidden = true;
                }
                item_initialize();
                //Initialize Timer
                timer  = al_create_timer(1.0/15.0);
                timer2  = al_create_timer(1.0);
                timer3  = al_create_timer(1.0/10.0);
                al_register_event_source(event_queue, al_get_timer_event_source(timer)) ;
                al_register_event_source(event_queue, al_get_timer_event_source(timer2)) ;
                al_register_event_source(event_queue, al_get_timer_event_source(timer3)) ;
                al_start_timer(timer);
                al_start_timer(timer2);
                al_start_timer(timer3);
                //draw blood initial
                blood_down_x = blood_top_x + blood_width;
                blood_down_y = blood_top_y + blood_height;
                blood_down_temp = blood_down_x;
                enemy_blood_down_x = blood_top_x + blood_between_distance + blood_width;
                blood_top_temp = blood_top_x + blood_between_distance;

                // 決定是否要在 setting時 input file
                // intput file
                if(input_file){
                    // read file
                    //change blood
                    input();
                }
            }
        }
    }
    else if(window == 2){   // Second window(Main Game)
        // Change Image for animation
        al_draw_bitmap(background, 0,0, 0);
        int i;
        if(ture) al_draw_bitmap(character1.image_path, character1.x, character1.y, 0);
        //blood
        //player

        if( blood_top_x <= blood_down_temp ){
            al_draw_rectangle(blood_top_x-0.5, blood_top_y-0.5, blood_down_x+1, blood_down_y+1, al_map_rgb(255, 255, 255), 1.0);
            al_draw_filled_rectangle(blood_top_x, blood_top_y, blood_down_temp, blood_down_y, al_map_rgb(255, 0, 0));
            blood_down_temp -= injury;
        }
        else{
            window = 3; // you lose
            is_death_anime = true;  // run death animation
            //al_rest(2.0);
        }
        injury = 0.0;
        //enermy
        if( enemy_blood_down_x >= blood_top_temp ){
            al_draw_rectangle(blood_top_x + blood_between_distance -0.5, blood_top_y-0.5, enemy_blood_down_x + 1, blood_down_y+1, al_map_rgb(255, 255, 255), 1.0);
            al_draw_filled_rectangle(blood_top_temp ,blood_top_y ,enemy_blood_down_x ,blood_down_y,al_map_rgb(255, 0, 0));
            blood_top_temp += enemy_injury;

        }
        else{
            window = 4;  // you win
        }
        enemy_injury = 0.0;

        for (i = 0; i < MAX_BULLET; i++){
            if(!bullets[i].hidden)
                al_draw_bitmap(bullets[i].image_path,bullets[i].x,bullets[i].y,0);
        }
        for(i = 0; i < MAX_BULLET_ENEMY; i++){
            if(!enemy_bullets[i].hidden)
                al_draw_bitmap(enemy_bullets[i].image_path,enemy_bullets[i].x,enemy_bullets[i].y,0);
        }

        draw_item();

        al_draw_textf(font,al_map_rgb(0,0,255),WIDTH / 3, 0, 0,"score : %d", score);

        if(dir) al_draw_bitmap(character2.image_path, character2.x, character2.y, 0);
        else al_draw_bitmap(character3.image_path, character2.x, character2.y, 0);



        al_flip_display();
        al_clear_to_color(al_map_rgb(0,0,0));

        // Listening for new event
        if (!al_is_event_queue_empty(event_queue)) {
            error = process_event();
        }
    }
    else if(window == 3){   // lose case
        al_draw_text(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2-50 , ALLEGRO_ALIGN_CENTRE, "You LOse!!");
        al_flip_display();
        if( is_death_anime )  death_animation(character1);
        ALLEGRO_EVENT event_r_e;
        al_wait_for_event(event_queue, &event_r_e);
        if(event_r_e.type == ALLEGRO_EVENT_KEY_DOWN){
            switch(event_r_e.keyboard.keycode){
                case ALLEGRO_KEY_TAB:
                    printf("restart\n");
                    blood_down_temp = blood_down_x;
                    window = 1;
                    judge_next_window = false;
                    return PLAY_AGAIN;
                    break;
                case ALLEGRO_KEY_B:
                    blood_down_temp = blood_down_x;
                    store = true;
                    break;
                case ALLEGRO_KEY_E:
                    return GAME_TERMINATE;
                    break;
                case ALLEGRO_KEY_ESCAPE:
                    return GAME_TERMINATE;
                    break;
                /*
                default:
                    al_flip_display();
                    al_rest(5.0);
                    al_draw_text(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "fuck you !");
                    al_flip_display();
                    return GAME_TERMINATE;
                    break;
                */
            }
        }
        al_flip_display();
    }
    else if(window == 4){   // win case
        al_draw_text(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2-50 , ALLEGRO_ALIGN_CENTRE, "You Win!!");
        al_flip_display();
        ALLEGRO_EVENT event_w;
        al_wait_for_event(event_queue, &event_w);
        if(event_w.type == ALLEGRO_EVENT_KEY_DOWN){
            switch(event_w.keyboard.keycode){
                case ALLEGRO_KEY_TAB:
                    printf("restart\n");
                    blood_down_temp = blood_down_x;
                    window = 1;
                    judge_next_window = false;
                    return PLAY_AGAIN;
                    break;
                case ALLEGRO_KEY_B:
                    blood_down_temp = blood_down_x;
                    store = true;
                    break;
                case ALLEGRO_KEY_E:
                    return GAME_TERMINATE;
                    break;
                case ALLEGRO_KEY_ESCAPE:
                    return GAME_TERMINATE;
                    break;
            }
        }
        al_flip_display();
    }
    else if(window == 6){
        //setting
        printf("enter setting\n");
        al_draw_bitmap(load_bitmap_at_size("setting_menu.png",WIDTH,HEIGHT), 0, 0, 0);
        al_flip_display();
        ALLEGRO_EVENT event_r_e;
        al_wait_for_event(event_queue, &event_r_e);
        if(event_r_e.type == ALLEGRO_EVENT_KEY_DOWN){
                printf("detect event\n");
            switch(event_r_e.keyboard.keycode){
                case ALLEGRO_KEY_1:
                    printf("play new music\n");
                    al_destroy_sample(song);
                    song = al_load_sample ( "chao_chao.flac" );
                    al_play_sample(song, 1.0, 0.0,1.0,ALLEGRO_PLAYMODE_LOOP,NULL);
                    break;
                case ALLEGRO_KEY_2:
                    al_destroy_sample(song);
                    song = al_load_sample ( "Perrywinkle.wav" );
                    al_play_sample(song, 1.0, 0.0,1.0,ALLEGRO_PLAYMODE_LOOP,NULL);
                    break;
                case ALLEGRO_KEY_3:
                    al_destroy_sample(song);
                    song = al_load_sample ( "Laugh_and_Cry.wav" );
                    al_play_sample(song, 1.0, 0.0,1.0,ALLEGRO_PLAYMODE_LOOP,NULL);
                    break;
                case ALLEGRO_KEY_I:
                    input();
                    break;
                case ALLEGRO_KEY_S:
                    out_file = true;
                    break;
                case ALLEGRO_KEY_BACKSPACE:
                    window = 1;
                    al_draw_bitmap(load_bitmap_at_size("menu.png",WIDTH,HEIGHT),0,0,0);
                    al_flip_display();
                    break;
                case ALLEGRO_KEY_ESCAPE:
                    return GAME_TERMINATE;
                    break;
                default:
                    al_clear_to_color(al_map_rgb(0,0,0));
                    al_draw_text(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "fuck you !");
                    al_flip_display();
                    al_rest(1.0);
                    break;
            }
        }
    }
    if(store){   // when we press B , we will go to store
        if(!al_is_event_queue_empty(event_queue)){
            printf("enter store\n");
            al_draw_bitmap(load_bitmap_at_size("store_menu.png",WIDTH,HEIGHT),0,0,0);
            al_draw_textf(font,al_map_rgb(0,0,255),WIDTH / 3, 0, 0,"score : %d", score);
            ALLEGRO_EVENT event_s;
            al_wait_for_event(event_queue, &event_s);
            if(event_s.type == ALLEGRO_EVENT_KEY_DOWN){
                printf("store\n");
                switch(event_s.keyboard.keycode){
                    case ALLEGRO_KEY_1:
                        if(score - 10 >= 0){
                            printf("buy_1\n");
                            //enemy_injury = 0;
                            //injury = 0;
                            bullet_addtional_v = 5;
                            MAX_COOLDOWN = 0.025;
                            window = 1;
                            store = false;
                            al_clear_to_color(al_map_rgb(0,0,0));
                            al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "rate of shot increase !");
                            al_flip_display();
                            al_rest(1.0);
                        }
                        else{
                            al_clear_to_color(al_map_rgb(0,0,0));
                            al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "Are you idiet ?");
                            al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+195 , ALLEGRO_ALIGN_CENTRE, "not enough money !");
                            al_flip_display();
                            al_rest(1.0);
                        }
                        //return PLAY_AGAIN;
                        break;
                    case ALLEGRO_KEY_2:
                        if(score - 50 >= 0){
                                enemy_addtional_v = -5;
                                enemy_MAX_COOLDOWN = 1;
                                window = 1;
                                store = false;
                                al_clear_to_color(al_map_rgb(0,0,0));
                                al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "rate of shot increase !");
                                al_flip_display();
                                al_rest(1.0);
                            }
                        else{
                                al_clear_to_color(al_map_rgb(0,0,0));
                                al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "Are you idiet ?");
                                al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+195 , ALLEGRO_ALIGN_CENTRE, "not enough money !");
                                al_flip_display();
                                al_rest(1.0);
                        }
                        //return PLAY_AGAIN;
                    break;
                    case ALLEGRO_KEY_3:
                        if(score - 10 >= 0){
                            //enemy_injury = 0;
                            //injury = 0;
                            bullet_addtional_v = 5;
                            MAX_COOLDOWN = 0.025;
                            window = 1;
                            store = false;
                            buy_add_bullet = true;
                            al_clear_to_color(al_map_rgb(0,0,0));
                            al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "rate of shot increase !");
                            al_flip_display();
                            al_rest(1.0);
                        }
                        else{
                            al_clear_to_color(al_map_rgb(0,0,0));
                            al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "Are you idiet ?");
                            al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+195 , ALLEGRO_ALIGN_CENTRE, "not enough money !");
                            al_flip_display();
                            al_rest(1.0);
                        }
                        break;
                    case ALLEGRO_KEY_4:
                        if(score - 10 >= 0){
                            printf("buy_4\n");
                            character1.vy = - (character1.vx);
                            window = 1;
                            store = false;
                            buy_add_bullet = true;
                            al_clear_to_color(al_map_rgb(0,0,0));
                            al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "numbers of bullet increase !");
                            al_flip_display();
                            al_rest(1.0);
                        }
                        else{
                            printf("can't_buy_4\n");
                            al_clear_to_color(al_map_rgb(0,0,0));
                            al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "Are you idiet ?");
                            al_draw_textf(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+195 , ALLEGRO_ALIGN_CENTRE, "not enough money !");
                            al_flip_display();
                            al_rest(1.0);
                        }
                        break;
                    case ALLEGRO_KEY_E:
                        return GAME_TERMINATE;
                        break;
                    case ALLEGRO_KEY_ESCAPE:
                        return GAME_TERMINATE;
                        break;
                    default:
                        al_flip_display();
                        al_rest(5.0);
                        al_draw_text(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+180 , ALLEGRO_ALIGN_CENTRE, "fuck you !");
                        return GAME_TERMINATE;
                        break;
                }
                al_flip_display();
            }
        }
    }
    return error;
}

void game_destroy() {
    // Make sure you destroy all things
    al_destroy_event_queue(event_queue);
    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_timer(timer2);
    al_destroy_bitmap(image);
    al_destroy_sample(song);
    al_destroy_sample(song_AGAIN);
}

ALLEGRO_BITMAP *load_bitmap_at_size(const char *filename, int w, int h)
{
  ALLEGRO_BITMAP *resized_bmp, *loaded_bmp, *prev_target;

  // 1. create a temporary bitmap of size we want
  resized_bmp = al_create_bitmap(w, h);
  if (!resized_bmp) return NULL;

  // 2. load the bitmap at the original size
  loaded_bmp = al_load_bitmap(filename);
  if (!loaded_bmp)
  {
     al_destroy_bitmap(resized_bmp);
     return NULL;
  }

  // 3. set the target bitmap to the resized bmp
  prev_target = al_get_target_bitmap();
  al_set_target_bitmap(resized_bmp);

  // 4. copy the loaded bitmap to the resized bmp
  al_draw_scaled_bitmap(loaded_bmp,
     0, 0,                                // source origin
     al_get_bitmap_width(loaded_bmp),     // source width
     al_get_bitmap_height(loaded_bmp),    // source height
     0, 0,                                // target origin
     w, h,                                // target dimensions
     0                                    // flags
  );

  // 5. restore the previous target and clean up
  al_set_target_bitmap(prev_target);
  al_destroy_bitmap(loaded_bmp);

  return resized_bmp;
}

void death_animation(Character obj){  //人物死亡特效
    float i,j;
    for(i = obj.y, j = 0; i > obj.y / 4; i -= obj.h / 4, j += 3){   // -/1 跌太少
        al_clear_to_color(al_map_rgb(0,0,0));
        al_draw_bitmap(obj.image_path, obj.x, i ,0);
        al_flip_display();
        al_rest(0.03);     // 0.02~0.1 is the best scene
        obj.image_path = load_bitmap_at_size("pegasus.png", obj.w - j, obj.h - j);
    }
    al_rest(0.5);
    for(i, j; i < HEIGHT ; i += obj.h / 4, j += 3){
        al_clear_to_color(al_map_rgb(0,0,0));
        al_draw_bitmap(obj.image_path , obj.x, i ,0);
        al_flip_display();
        al_rest(0.03);
        obj.image_path = load_bitmap_at_size("pegasus.png", obj.w - j, obj.h - j);
    }
    obj.image_path = load_bitmap_at_size("pegasus.png", obj.w, obj.h);
    is_death_anime = false;
}

void play_music(){
    song_AGAIN = al_load_sample( "Sunshine.wav" );
    if (!song){
        printf( "Audio clip sample not loaded!\n" );
        show_err_msg(-6);
    }
    // Loop the song until the display closes
    al_play_sample(song_AGAIN, 0.5, 0.0,1.0,ALLEGRO_PLAYMODE_LOOP,NULL);
}

void input(){
    scanf("%f", &blood_down_temp);
    scanf("%f", &enemy_blood_down_x);
    scanf("%f", &enemy_injury);
    scanf("%d", &score);
}

void output(){
    printf("%f\n", blood_down_temp);
    printf("%f\n", enemy_blood_down_x);
    printf("%f\n", enemy_injury);
    printf("%d\n",score);
}

void item_initialize(){
    int i;
    for(i = 0; i < 2; ++i){
        special_item_speed[i].w = 40;
        special_item_speed[i].h = 40;
        special_item_speed[i].image_path = load_bitmap_at_size("bullet.png",special_item_speed[i].w,special_item_speed[i].h);
        special_item_speed[i].vx = -20 ;
        special_item_speed[i].vy = (rand() % 41) - 20;
        special_item_speed[i].hidden = true;
        special_item_speed[i].x = WIDTH;
        special_item_speed[i].y = rand() % HEIGHT;
    }
    for(i = 0; i < 2; ++i){
        special_item_injury[i].w = 40;
        special_item_injury[i].h = 40;
        special_item_injury[i].image_path = load_bitmap_at_size("bullet.png",special_item_injury[i].w,special_item_injury[i].h);
        special_item_injury[i].vx = -20 ;
        special_item_injury[i].vy = (rand() % 41) - 20;
        special_item_injury[i].hidden = true;
        special_item_injury[i].x = WIDTH;
        special_item_injury[i].y = rand() % HEIGHT;
    }
    for(i = 0; i < 10; ++i){
        special_item_score[i].w = 40;
        special_item_score[i].h = 40;
        special_item_score[i].image_path = load_bitmap_at_size("bullet.png",special_item_score[i].w,special_item_score[i].h);
        special_item_score[i].vx = -20 ;
        special_item_score[i].vy = (rand() % 41) - 20;
        special_item_score[i].hidden = true;
        special_item_score[i].x = WIDTH;
        special_item_score[i].y = rand() % HEIGHT;
    }
    for(i = 0; i < 3; ++i){
        special_item_blood[i].w = 40;
        special_item_blood[i].h = 40;
        special_item_blood[i].image_path = load_bitmap_at_size("bullet.png",special_item_blood[i].w,special_item_blood[i].h);
        special_item_blood[i].vx = -20 ;
        special_item_blood[i].vy = (rand() % 41) - 20;
        special_item_blood[i].hidden = true;
        special_item_blood[i].x = WIDTH;
        special_item_blood[i].y = rand() % HEIGHT;
    }
}

void update_item(){
    int i;
    for(i = 0; i < 2; ++i){
        if (special_item_speed[i].hidden)
                continue;
            special_item_speed[i].x += special_item_speed[i].vx;
            special_item_speed[i].y += special_item_speed[i].vy;
            if (special_item_speed[i].x < 0)
                special_item_speed[i].hidden = true;
            if(special_item_speed[i].y > 0 || special_item_speed[i].y > HEIGHT)
                special_item_speed[i].vy *= -1;
    }
    for(i = 0; i < 2; ++i){
        if (special_item_injury[i].hidden)
                continue;
            special_item_injury[i].x += special_item_injury[i].vx;
            special_item_injury[i].y += special_item_injury[i].vy;
            if (special_item_injury[i].x < 0)
                special_item_injury[i].hidden = true;
            if(special_item_injury[i].y < 0 || special_item_injury[i].y > HEIGHT)
                special_item_injury[i].vy *= -1;
    }
    for(i = 0; i < 10; ++i){
        if (special_item_score[i].hidden)
                continue;
            special_item_score[i].x += special_item_score[i].vx;
            special_item_score[i].y += special_item_score[i].vy;
            if (special_item_score[i].x < 0)
                special_item_score[i].hidden = true;
            if(special_item_score[i].y < 0 || special_item_score[i].y > HEIGHT)
                special_item_score[i].vy *= -1;

    }
    for(i = 0; i < 3; ++i){
        if (special_item_blood[i].hidden)
                continue;
            special_item_blood[i].x += special_item_blood[i].vx;
            special_item_blood[i].y += special_item_blood[i].vy;
            if (special_item_blood[i].x < 0)
                special_item_blood[i].hidden = true;
            if(special_item_blood[i].y < 0 || special_item_blood[i].y > HEIGHT)
                special_item_blood[i].vy *= -1;
    }
}

void eat_item(){
    int i;

    for(i = 0; i < 2; ++i){
        if((special_item_speed[i].x > character1.x) && (special_item_speed[i].x < character1.x + character1.w)
               &&(special_item_speed[i].y > character1.y) && (special_item_speed[i].y < character1.y + character1.h)){
                bullet_addtional_v = 5;
                special_item_speed[i].hidden = true;
                break;
        }
    }
    for(i = 0; i < 2; ++i){
        if((special_item_injury[i].x > character1.x) && (special_item_injury[i].x < character1.x + character1.w)
               &&(special_item_injury[i].y > character1.y) && (special_item_injury[i].y < character1.y + character1.h)){
                bullet_addtional_injury = 4;
                special_item_injury[i].hidden = true;
                break;
        }
    }
    for(i = 0; i < 10; ++i){
        if((special_item_score[i].x > character1.x) && (special_item_score[i].x < character1.x + character1.w)
               &&(special_item_score[i].y > character1.y) && (special_item_score[i].y < character1.y + character1.h)){
                score += 3;
                special_item_score[i].hidden = true;
                break;
        }
    }
    for(i = 0; i < 3; ++i){
        if((special_item_blood[i].x > character1.x) && (special_item_blood[i].x < character1.x + character1.w)
               &&(special_item_blood[i].y > character1.y) && (special_item_blood[i].y < character1.y + character1.h)){
                if(blood_down_temp < blood_down_x)
                blood_down_temp += 10;
                special_item_blood[i].hidden = true;
                break;
        }
    }
}

void draw_item(){
    int i;

    for(i = 0; i < 2; ++i){
        if(!special_item_speed[i].hidden)
                al_draw_bitmap(special_item_speed[i].image_path,special_item_speed[i].x,special_item_speed[i].y,0);
    }
    for(i = 0; i < 2; ++i){
        if(!special_item_injury[i].hidden)
                al_draw_bitmap(special_item_injury[i].image_path,special_item_injury[i].x,special_item_injury[i].y,0);
    }
    for(i = 0; i < 10; ++i){
        if(!special_item_score[i].hidden){
             al_draw_bitmap(special_item_score[i].image_path,special_item_score[i].x,special_item_score[i].y,0);
        }

    }
    for(i = 0; i < 3; ++i){
        if(!special_item_blood[i].hidden)
                al_draw_bitmap(special_item_blood[i].image_path,special_item_blood[i].x,special_item_blood[i].y,0);
    }
}


