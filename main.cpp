#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

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
    float vx,vy;
    bool hidden;
    ALLEGRO_BITMAP *image_path;

}Character;

Character character1;
Character character2;
Character character3;
character bullets[MAX_BULLET];
character enemy_bullets[MAX_BULLET_ENEMY];

int imageWidth = 0;
int imageHeight = 0;
int draw = 0;
int done = 0;
int window = 1;
bool judge_next_window = false;
bool ture = true; //true: appear, false: disappear
bool next = false; //true: trigger
bool dir = true; //true: left, false: right
bool *keys;
bool *mouse_state;
int injury_time = 0;
bool is_injury = true;
bool store = false;
bool is_death_anime = false;  // 偵測是否跑過死亡動畫
//following code is to define the item we buy from shop
int bullet_addtional_v = 0;  // let players bullet faster
int enemy_addtional_v = 0;   // let enemy slower
bool buy_add_bullet = true;

void show_err_msg(int msg);
void game_init();
void game_begin();
int process_event();
int game_run();
void game_destroy();
void death_animation(Character);

int main(int argc, char *argv[]) {
    int msg = 0;

    game_init();
    //game_log("Game initialized");
    game_begin();
    //game_log("Allegro5 initialized");
    //game_log("Game begin");
    while (msg != GAME_TERMINATE) {
        //game_log("Game start event loop");
        msg = game_run();
        if (msg == GAME_TERMINATE)
            //game_log("Game end");
            printf("Game Over\n");
        else if(msg == PLAY_AGAIN){
            game_init();
            game_begin();
        }
    }

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
    //al_clear_to_color(al_map_rgb(100,100,100));
    // Load and draw text
    font = al_load_ttf_font("pirulen.ttf",12,0);
    //al_draw_text(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2+220 , ALLEGRO_ALIGN_CENTRE, "Press 'Enter' to start");
    //al_draw_rectangle(WIDTH/2-150, 510, WIDTH/2+150, 550, al_map_rgb(255, 255, 255), 0);
    al_draw_bitmap(load_bitmap_at_size("menu.png",WIDTH,HEIGHT),0,0,0);
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
                case ALLEGRO_KEY_B:
                    store = true;
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
                // For Start
                case ALLEGRO_KEY_ENTER:
                    judge_next_window = true;
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
                injury = 1.0;
                enemy_bullets[i].hidden = true;
                break;
            }
        }

        // enemy injury
        for( i = 0; i < MAX_BULLET; ++i){
            if((bullets[i].x > character2.x) && (bullets[i].x < character2.x + character2.w)
               &&(bullets[i].y > character2.y) && (bullets[i].y < character2.y + character2.h)){
                enemy_injury = 1.0;
                bullets[i].hidden = true;
                break;
            }
        }
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
                    enemy_bullets[i].image_path = load_bitmap_at_size("bulletbt.png",bullets[i].w,bullets[i].h);
                    enemy_bullets[i].vx = -20;
                    enemy_bullets[i].vy = 0;
                    enemy_bullets[i].hidden = true;
                }
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
            }
        }
    }
    else if(window == 2 && !store){   // Second window(Main Game)
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
        al_draw_text(font, al_map_rgb(255,255,255), WIDTH/2, HEIGHT/2-50 , ALLEGRO_ALIGN_CENTRE, "You Win!!");
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
    if(store){   // when we press B , we will go to store
        printf("enter store\n");
        al_draw_bitmap(load_bitmap_at_size("store_menu.png",WIDTH,HEIGHT),0,0,0);
        ALLEGRO_EVENT event_s;
        al_wait_for_event(event_queue, &event_s);
        if(event_s.type == ALLEGRO_EVENT_KEY_DOWN){
            printf("store\n");
            switch(event_s.keyboard.keycode){
                case ALLEGRO_KEY_1:
                    enemy_injury = 0;
                    injury = 0;
                    bullet_addtional_v = 5;
                    MAX_COOLDOWN = 0.025;
                    window = 1;
                    store = false;
                    buy_add_bullet = true;
                    printf("window %d injury %d enemy injury %d\n",window,injury,enemy_injury);
                    return PLAY_AGAIN;
                    break;
                case ALLEGRO_KEY_2:
                    enemy_addtional_v = -5;
                    enemy_MAX_COOLDOWN = 1;
                    window = 1;
                    store = false;
                    printf("2\n");
                    return PLAY_AGAIN;
                    break;
                case ALLEGRO_KEY_3:
                    bullet_addtional_v = -5;
                    enemy_MAX_COOLDOWN = 1;
                    window = 1;
                    store = false;
                    return PLAY_AGAIN;
                    break;
                case ALLEGRO_KEY_4:
                    bullet_addtional_v = -5;
                    enemy_MAX_COOLDOWN = 1;
                    window = 1;
                    store = false;
                    return PLAY_AGAIN;
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

// +=================================================================+
// | Code below is for debugging purpose, it's fine to remove it.    |
// | Deleting the code below and removing all calls to the functions |
// | doesn't affect the game.                                        |
// +=================================================================+

void game_abort(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    game_vlog(format, arg);
    va_end(arg);
    fprintf(stderr, "error occured, exiting after 2 secs");
    // Wait 2 secs before exiting.
    al_rest(2);
    // Force exit program.
    exit(1);
}

void game_log(const char* format, ...) {
#ifdef LOG_ENABLED
    va_list arg;
    va_start(arg, format);
    game_vlog(format, arg);
    va_end(arg);
#endif
}

void game_vlog(const char* format, va_list arg) {
#ifdef LOG_ENABLED
    static bool clear_file = true;
    vprintf(format, arg);
    printf("\n");
    // Write log to file for later debugging.
    FILE* pFile = fopen("log.txt", clear_file ? "w" : "a");
    if (pFile) {
        vfprintf(pFile, format, arg);
        fprintf(pFile, "\n");
        fclose(pFile);
    }
    clear_file = false;
#endif
}
