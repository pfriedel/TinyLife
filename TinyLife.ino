//TinyLife
//Conway's Game of Life runing on 5-pin charlieplexed LED grid (20 LEDs) on ATTiny85
// 
// heavily inspired by Ben Brandt's TinyChuck5
// http://b2ben.blogspot.com/2011/02/diving-into-microcontrollers-my-tiny.html
//
// I've squashed some of his quirks and introduced all new ones, I'm sure.


#define PB0 0 //Pin 5 (PB0) on ATtiny85
#define PB1 1 //Pin 6 (PB1) on ATtiny85
#define PB2 2 //Pin 7 (PB2) on ATtiny85
#define PB3 3 //Pin 2 (PB3) on ATtiny85
#define PB4 4 //Pin 3 (PB4) on ATtiny85

#define COLS 5 // usually x
#define ROWS 4 // usually y

#define MAX_SHADE 100

void setup() {
  uint16_t seed=0;
  uint8_t count=32;
  while (--count) {
    seed = (seed<<1) | (analogRead(1)&1);
  }
  randomSeed(seed);
}

void loop() {
  Life();
}

uint16_t world[COLS][ROWS][2];
uint16_t frame_log[COLS][ROWS];
uint8_t x, y;

void Life() {
  uint16_t frame_number = 0;
  uint16_t f;

  initialize_frame_log(); // blank out the frame_log world
  
  // flash the screen - ~1000msec
  for(y = 0; y < ROWS; y++) { for(x = 0; x < COLS; x++) { world[x][y][1] = MAX_SHADE; } }

  fade_to_next_frame();
  for(f = 0; f<1000; f++) { draw_frame(); }

  // draw the initial generation
  set_initial_frame();
  fade_to_next_frame();

  while(1) {
    // Log every 20th frame to monitor for repeats
    if( frame_number == 0 ) { 
      log_current_frame(); 
    }
    
    for(f = 0; f<1000; f++ ) { draw_frame(); }
    
    // generate the next generation
    generate_next_generation();

    // Death due to still life
    // if there are no changes between the current generation and the next generation (still life), break out of the loop.
    if( current_equals_next() == 1 ) {
      for(f = 0; f<1500; f++ ) { draw_frame(); }
      break;
    }
    
    // Death due to oscillator
    // If the next frame is the same as a frame from 20 generations ago, we're in a loop.
    if( next_equals_logged_frame() == 1 ) {
      for(f = 0; f<1500; f++) { draw_frame(); }
      break;
    }

    // Otherwise, fade to the next generation
    fade_to_next_frame();

    frame_number++;

    if(frame_number >= 20 ) {
      frame_number = 0;
    }
  }
}

void initialize_frame_log() {
  for(y=0; y < ROWS; y++) {
    for(x=0; x < COLS; x++) {
      frame_log[x][y] = -1;
    }
  }
}

void log_current_frame() {
  for(y=0; y < ROWS; y++) {
    for(x=0; x < COLS; x++) {
      frame_log[x][y] = world[x][y][0];
    }
  }
}

void set_initial_frame(void) {
  uint8_t density = random(40,80);
  for(y=0; y<ROWS; y++) {
    for(x=0; x<COLS; x++) {
      if(random(100) > density) {
        world[x][y][1] = MAX_SHADE;
      }
      else {
	world[x][y][1] = 0; 
      }
    }
  }
}

uint8_t current_equals_next() {
  for(y=0; y<ROWS; y++) {
    for(x=0; x<COLS; x++) {
      if( world[x][y][0] != world[x][y][1] ) {
        return 0;
      }
    }
  }
  return 1;
}

void generate_next_generation(void){  //looks at current generation, writes to next generation array
  uint8_t neighbors;
  for ( y=0; y<ROWS; y++ ) {
    for ( x=0; x<COLS; x++ ) {
      //count the number of current neighbors
      neighbors = 0;
      if( get_led_xy((x-1),(y-1)) > 0 ) { neighbors++; } //NW
      if( get_led_xy(( x ),(y-1)) > 0 ) { neighbors++; } //N
      if( get_led_xy((x+1),(y-1)) > 0 ) { neighbors++; } //NE
      if( get_led_xy((x-1),( y )) > 0 ) { neighbors++; } //W
      if( get_led_xy((x+1),( y )) > 0 ) { neighbors++; } //E
      if( get_led_xy((x-1),(y+1)) > 0 ) { neighbors++; } //SW
      if( get_led_xy(( x ),(y+1)) > 0 ) { neighbors++; } //S
      if( get_led_xy((x+1),(y+1)) > 0 ) { neighbors++; } //SE

      //current cell is alive
      if( world[x][y][0] > 0 ){ 
	//Any live cell with fewer than two live neighbours dies, as if caused by under-population
        if( neighbors < 2 ){    
          world[x][y][1] = 0;
        } 
	//Any live cell with two or three live neighbours lives on to the next generation
	if( (neighbors == 2) || (neighbors == 3) ){
	  world[x][y][1] = MAX_SHADE;
        }
	//Any live cell with more than three live neighbours dies, as if by overcrowding
        if( neighbors > 3 ){
          world[x][y][1] = 0;
        }
      }
      //current cell is dead
      else {
	// Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction
        if( neighbors == 3 ){
          world[x][y][1] = MAX_SHADE;
        }
	//stay dead for next generation
        else {
          world[x][y][1] = 0;
        }
      }
    }
  }
}

uint8_t next_equals_logged_frame(){
  for(y = 0; y<ROWS; y++) {
    for(x = 0; x<COLS; x++) {
      if(world[x][y][1] != frame_log[x][y] ) {
        return 0;
      }
    }
  }
  return 1;
}

void resetDisplay() {
  for(y = 0; y <=ROWS; y++) {
    for(x = 0; x <=COLS; x++) {
      world[x][y][1] = 0;
    }
  }
}

uint8_t get_led_xy (int8_t col, int8_t row) { // signed matters.
  if(col < 0 | col > COLS-1) { return 0; }
  if(row < 0 | row > ROWS-1) { return 0;  }

  return world[col][row][0];
}

//DDRB direction config for each LED (1 = output)
const int8_t led_dir[20] = {
  // ( output pin | input pin )
  ( 1<<PB3 | 1<<PB2 ), //LED 0
  ( 1<<PB4 | 1<<PB2 ), //LED 1
  ( 1<<PB0 | 1<<PB2 ), //LED 2
  ( 1<<PB1 | 1<<PB2 ), //LED 3
  ( 1<<PB2 | 1<<PB1 ), //LED 4
  
  ( 1<<PB3 | 1<<PB1 ), //LED 5
  ( 1<<PB4 | 1<<PB1 ), //LED 6
  ( 1<<PB0 | 1<<PB1 ), //LED 7
  ( 1<<PB1 | 1<<PB0 ), //LED 8
  ( 1<<PB2 | 1<<PB0 ), //LED 9
  
  ( 1<<PB3 | 1<<PB0 ), //LED 10
  ( 1<<PB4 | 1<<PB0 ), //LED 11
  ( 1<<PB0 | 1<<PB4 ), //LED 12
  ( 1<<PB1 | 1<<PB4 ), //LED 13
  ( 1<<PB2 | 1<<PB4 ), //LED 14
  
  ( 1<<PB3 | 1<<PB4 ), //LED 15
  ( 1<<PB4 | 1<<PB3 ), //LED 16
  ( 1<<PB0 | 1<<PB3 ), //LED 17
  ( 1<<PB1 | 1<<PB3 ), //LED 18
  ( 1<<PB2 | 1<<PB3 )  //LED 19
};

//PORTB output config for each LED (1 = High, 0 = Low)
const int8_t led_out[20] = {
  ( 1<<PB3 ), //LED 0
  ( 1<<PB4 ), //LED 1
  ( 1<<PB0 ), //LED 2
  ( 1<<PB1 ), //LED 3
  ( 1<<PB2 ), //LED 4
  
  ( 1<<PB3 ), //LED 5
  ( 1<<PB4 ), //LED 6
  ( 1<<PB0 ), //LED 7
  ( 1<<PB1 ), //LED 8
  ( 1<<PB2 ), //LED 9
  
  ( 1<<PB3 ), //LED 10
  ( 1<<PB4 ), //LED 11
  ( 1<<PB0 ), //LED 12
  ( 1<<PB1 ), //LED 13
  ( 1<<PB2 ), //LED 14
  
  ( 1<<PB3 ), //LED 15
  ( 1<<PB4 ), //LED 16
  ( 1<<PB0 ), //LED 17
  ( 1<<PB1 ), //LED 18
  ( 1<<PB2 )  //LED 19
};

void light_led(uint8_t led_num) { //led_num must be from 0 to 19
  DDRB = led_dir[led_num];
  PORTB = led_out[led_num];
}

void leds_off() {
  DDRB = 0;
  PORTB = 0;	
}

void draw_frame(void){
  uint8_t led, bright_val;
  for(y=0; y < ROWS; y++) { 
    for(x=0; x < COLS; x++) { 

      bright_val = world[x][y][0];
      led = (x + (y*5)); // devolve the array into a line
      for(uint8_t b=0 ; b < bright_val ; b+=4 ) { light_led(led); } // light while the percentage on
      for(uint8_t b=bright_val ; b<MAX_SHADE ; b+=4 ) { leds_off(); } // off while the percentage off
    } 
  }
  // Force the LEDs off - otherwise if the last LED on (LED 19) was at full
  // brightness, it would never get turned off and it would flicker.
  leds_off(); 
}

void fade_to_next_frame() {
  uint8_t changes;
  
  while(1) {
    changes = 0;
    for(y = 0; y < ROWS; y++) {
      for(x = 0; x < COLS; x++) {
        if( world[x][y][0] < world[x][y][1] ) { world[x][y][0]++; changes++; }
        if( world[x][y][0] > world[x][y][1] ) { world[x][y][0]--; changes++; }
      }
    }
    // give the fade a chance to draw for a moment.
    for(uint8_t f = 0; f<3; f++) { draw_frame(); }

    if( changes == 0 ) {
      break;
    }
  }
}
