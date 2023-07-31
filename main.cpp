#include <stdio.h>
#include <string.h>
#include <iostream>
#include <bitset>
#include <cstdlib>
#include <time.h>
#include <fstream>
#include <SDL2/SDL.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 320;

using namespace std;

const size_t SYSTEM_MEMORY = 4095;
//holds functions and values for the chip-8 interpreter

const size_t START_ADDRESS = 511;
const size_t FONT_DATA_START = 0x050;
const size_t FONT_DATA_END = 0x09F;

const unsigned int FONT_ADDRESSES[0x11] =
  {
    0x050, 0x55, 0x5a, 0x5f,
    0x64, 0x69, 0x6e, 0x73,
    0x78, 0x7d, 0x82, 0x87,
    0x8c, 0x91, 0x96, 0x9b,
    
  };

//colors
int OFF_R, OFF_G, OFF_B, ON_R, ON_G, ON_B;

//takes position of pixel, and then draws a 10*10 rectangle to
//represent it
void drawPixel(int x, int y, SDL_Renderer* &renderer){
  SDL_Rect pxl = {x, y, 10, 10};
  //on color = mellow yellow
  SDL_SetRenderDrawColor(renderer, ON_R, ON_G, ON_B, 0xFF);
  //SDL_SetRenderDrawColor(renderer, 0x2F, 0xFF, 0x00, 0xFF);
  SDL_RenderFillRect(renderer, &pxl);
  //printf("render pixel at %i, %i", x, y);
}

//flags
bool DEBUG_MODE = false;
bool COSMAC_VP_MODE = false;

void setCharacter(unsigned char memory[], unsigned int c, unsigned char x, unsigned char x2, unsigned char x3, unsigned char x4, unsigned char x5){
  memory[c] = x;
  memory[c+1] = x2;
  memory[c+2] = x3;
  memory[c+3] = x4;
  memory[c+4] = x5;    
}

void setOffColor(int r, int g, int b){
  OFF_R = r;
  OFF_G = g;
  OFF_B = b;
}

void setOnColor(int r, int g, int b){
  ON_R = r;
  ON_G = g;
  ON_B = b;
}

void initializeFont(unsigned char memory[]){
  //memory is stored from 0x050 to 0x09F
  unsigned int character = 0x0;
  unsigned int c = FONT_DATA_START;
  while (character < 0xf){
    switch(character){
    case 0x0:{//0x050
      setCharacter(memory, c, 0xF0, 0x90, 0x90, 0x90, 0xF0);
      break;}
    case 0x1:{//0x055
      setCharacter(memory, c, 0x20, 0x60, 0x20, 0x20, 0x70);
      break;}
    case 0x2:{//0x05A
      setCharacter(memory, c, 0xF0, 0x10, 0xF0, 0x80, 0xF0);
      break;}
    case 0x3:{//0x05F
      setCharacter(memory, c, 0xF0, 0x10, 0xF0, 0x10, 0xF0);
      break;}
    case 0x4:{//0x064
      setCharacter(memory, c, 0x90, 0x90, 0xF0, 0x10, 0x10);
      break;}
    case 0x5:{//0x069
      setCharacter(memory, c, 0xF0, 0x80, 0xF0, 0x10, 0xF0);
      break;}
    case 0x6:{//0x06E
      setCharacter(memory, c, 0xF0, 0x80, 0xF0, 0x90, 0xF0);
      break;}
    case 0x7:{//0x73
      setCharacter(memory, c, 0xF0, 0x10, 0x20, 0x40, 0x40);
      break;}
    case 0x8:{//0x78
      setCharacter(memory, c, 0xF0, 0x90, 0xF0, 0x90, 0xF0);
      break;}
    case 0x9:{//0x7d
      setCharacter(memory, c, 0xF0, 0x90, 0xF0, 0x10, 0xF0);
      break;}
    case 0xa:{//0x82
      setCharacter(memory, c, 0xF0, 0x90, 0xF0, 0x90, 0x90);
      break;}
    case 0xb:{//0x87
      setCharacter(memory, c, 0xE0, 0x90, 0xE0, 0x90, 0xE0);
      break;}
    case 0xc:{//0x8c
      setCharacter(memory, c, 0xF0, 0x80, 0x80, 0x80, 0xF0);
      break;}
    case 0xd:{//0x91
      setCharacter(memory, c, 0xE0, 0x90, 0x90, 0x90, 0xE0);
      break;}
    case 0xe:{//0x96
      setCharacter(memory, c, 0xF0, 0x80, 0xF0, 0xF0, 0xF0);
      break;}
    case 0xf:{//0x9b
      setCharacter(memory, c, 0x80, 0xF0, 0xF0, 0x80, 0x80);
      break;}
    }
    c += 5;
    ++character;
  }  
}

unsigned int getKey(unsigned int sc){
  //printf("%i\n", sc);
  switch(sc){
    case 30:{return 0x1;break;}
    case 31:{return 0x2;break;}
    case 32:{return 0x3;break;}
    case 33:{return 0xc;break;}
    case 20:{return 0x4;break;}
    case 26:{return 0x5;break;}
    case 8:{return 0x6;break;}
    case 21:{return 0xd;break;}
    case 4:{return 0x7;break;}
    case 22:{return 0x8;break;}
    case 7:{return 0x9;break;}
    case 9:{return 0xe;break;}
    case 29:{return 0xa;break;}
    case 27:{return 0x0;break;}
    case 6:{return 0xb;break;}
    case 25:{return 0xf;break;}
    case 44:{
      if (DEBUG_MODE){
	return 0x9F;
      }
      break;}
  }
  return -1;
	  
}

void close(SDL_Window* &window, SDL_Renderer* &renderer){
  SDL_DestroyWindow(window);
  window = NULL;

  SDL_DestroyRenderer(renderer);
  renderer = NULL;

    
  SDL_Quit();
}


int main(int argc, char* argv[]){  
  //programs are loaded to memory at address 0x200 (512)
  unsigned char mem[SYSTEM_MEMORY] = {0};
  //set up font
  initializeFont(mem);
  
  ifstream f;

  if (argc <= 1) {
    printf("no program file specified!\n");
    return -1;
  }

  //printf("argc: %i\n", argc);

  if (argc >= 2){
    for (size_t o = 0; o < argc; ++o){
      //printf("%s\n", argv[o]);
      
      if (strcmp(argv[o], "-d") == 0){
	//printf("debug mode on\n");
	DEBUG_MODE = true;
      }
      
      if (strcmp(argv[o], "-h") == 0){
	COSMAC_VP_MODE = true;
      }
      
    }
  }

  //yellow and blue color scheme
  setOnColor(248, 222, 126);
  setOffColor(34, 76, 152);

  //matrix colors
  // setOnColor(0, 255, 0);
  //setOffColor(0, 0, 0);
  
  //printf("%b\n", DEBUG_MODE);
  //open file
  f.open(argv[argc - 1], ios::binary);
  
  if (f.fail()) {
    std::cerr << "invalid file!\n";
    return -1;
  }

  size_t idx = START_ADDRESS;

  //read file
  //(change to either take input or start the program in a "programming" mode)
  while (!f.eof()) {
    char byte;

    f.get(static_cast<char&>(byte));

    if (f.fail()) {
      std::cerr << "failed to read byte at index " << idx << "!\n";
      break;
    }
    
    mem[idx] = byte;
    
    idx += 1;
  }

  /* print loaded memory

  for (size_t i = 0; i < 4000; ++i){

    if (!(!(mem[i]) && !(mem[i + 1]))){
      printf("%i: %x, ", i, mem[i]);
      if (!(i % 8))
       printf("\n");
    }
  }
  */
  srand(time(0));
  //open an SDL window, intialize main execution loop
  SDL_Renderer *renderer;
  SDL_Window *window;
  
  if (SDL_Init(SDL_INIT_VIDEO) != 0){
    printf("SDL_Init Error: %s\n", SDL_GetError());
    return 1;
  }
  SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
  
  if (!renderer | !window) {
    printf("window and renderer init failed %s\n", SDL_GetError());
    close(window, renderer);
    return 1;
  }

  SDL_SetWindowBordered(window, SDL_FALSE);
  
  bool quit = false;

  //other important variables
  unsigned int PC = START_ADDRESS;
  unsigned char vreg[0xF] = {0};
  unsigned int I;
  unsigned int stack[0xF] = {0};
  bool display[63][31] = {};
  unsigned char soundTimer = 0;
  unsigned char delayTimer = 0;
  
  while(!quit){

    //fetch instructions
    unsigned char i;
    //find starting bit to get type of instruction
    i = (mem[PC] & (0xF0)) >> 4;
    
    //store the usuable values of each code
    unsigned int NNN = 0 | (((mem[PC] & 0xF) << 8) | (mem[PC + 1] & 0xF0) | (mem[PC + 1] & 0xF));
    unsigned char NN = mem[PC + 1];
    unsigned char N = (mem[PC + 1] & 0xF);

    //used to access registers
    unsigned char X = (mem[PC] & 0xF);
    unsigned char Y = ((mem[PC + 1] & 0xF0) >> 4);

    //printf("NN: %x\n", NN);

    SDL_Event e;

    int pressedKey = -1;
    
    while(SDL_PollEvent(&e) != 0)
      {
	switch(e.type){
	case SDL_QUIT:{
	  quit = true;
	  break;
	}
	case SDL_KEYDOWN:{
	  //check for the scancodes 1 - V on keyboard
	  //1: 30 2:31 3:32 4:33
	  //q: 20 w: 26 e: 8 r: 21
	  //a: 4 s: 22 d: 7 f: 9
	  //z: 29 x: 27 c: 6 v: 25
	  //add ability to support other mappings
	  //space = 0x9F
	  unsigned int sc = e.key.keysym.scancode;
	  pressedKey = getKey(sc);
	  // printf("key pressed: %x\n", pressedKey);
	  break;
	}
	}
      }

    if (DEBUG_MODE){
      if (pressedKey != 0x9F){
	continue;
      }
    }
    
    PC += 2;
    //decode instructions

    if (DEBUG_MODE)
      printf("decoding instruction %x%x\n", i, NNN);
    
    switch(i){
    case 0x0:{
      //clear screen
      switch(NNN){
      case 0x0e0:{

	  if (DEBUG_MODE)
	    printf("clear screen\n");

	  for (size_t w = 0; w < 63; ++w){
	    for (size_t h = 0; h < 31; ++h){
	      display[w][h] = 0;
	    }
	  }
	  break;
	}
      case 0x0ee: {
	  size_t sp = 0xF;
	  while (sp >= 0){

	    if (DEBUG_MODE)
	      printf("location at stack %i: %x\n", sp, stack[sp]);

	    if (stack[sp] > 0){
	      PC = stack[sp];
	      stack[sp] = 0;

	      if(DEBUG_MODE)
		printf("return from subroutine at %i\n", sp);

	      break;
	    }
	    else{
	    sp -= 1;
	    }
	  }
	  break;
	}
      case 0x000:{
	  printf("reached uninitialized memory!\n");
	  break;
	}
      }
      
      break;
    }
    case 0x1:{
      //jump
      //remember to check for an off by one error!
      PC = (NNN-1);
      //printf("jump to %x\n", NNN-1);
      break;
    }
    case 0x2:{
      //call subroutine at location NNN
      for (size_t sp = 0; sp < 0xF; ++sp){
	if (stack[sp] == 0){
	  stack[sp] = PC;

	  if (DEBUG_MODE)
	    printf("storing subroutine at sp %i\n", sp);

	  break;
	}
	if (sp > 0xF)
	  printf("stack overflow!\n");
      }

      PC = (NNN - 1);
      
      break;
    }
    case 0x3:{
      //skip 1 instruction if VX == NN
      if (vreg[X] == NN){
	PC += 2;
      }
      break;
    }
    case 0x4:{
      //skip next instruction if VX != NN
      if (DEBUG_MODE)
	printf("X%x = %x, NN = %x\n",X, vreg[X], NN);
      
      if (vreg[X] != NN){
	PC += 2;
      }
      break;
    }
    case 0x5:{
      //skip if VX and VY are equal
      if (vreg[X] == vreg[Y])
	PC += 2;
      break;
    }
    case 0x6:{
      vreg[X] = NN;
      //printf("set register %i to %i\n", X, NN);
      //set register vx
      break;
    }
    case 0x7:{
      vreg[X] += NN;
      //printf("add %i to register %i\n", NN, X);
      //add value to register VX
      break;
    }
    case 0x8:{ //logical and arithmetic instructions
      switch(N){
      case 0x0:{
	vreg[X] = vreg[Y];
	break;
      }
      case 0x1:{
	vreg[X] = vreg[X] | vreg[Y];
	break;
      }
      case 0x2:{
	vreg[X] = vreg[X] & vreg[Y];
	break;
      }
      case 0x3:{
	vreg[X] = vreg[X] ^ vreg[Y];
	break;
      }
      case 0x4:{
	if ((vreg[X] + vreg[Y]) > 255){
	  vreg[0xF] = 1;
	}
	else{
	  vreg[0xF] = 0;
	}
	
	if(DEBUG_MODE){
	  printf("adding X%x to Y%x: %x + %x = %x (carry flag -> %b)\n", X, Y, vreg[X], vreg[Y], vreg[X] + vreg[Y] ,vreg[0xF]); 
	}
	
	vreg[X] = (vreg[X] + vreg[Y]);

	if (DEBUG_MODE)
	  printf("result: %x\n", vreg[X]);
	break;
      }
      case 0x5:{
	//VX - VY
	if (vreg[X] > vreg[Y])
	  vreg[0xF] = 1;
	else
	  vreg[0xF] = 0;
	
	vreg[X] = vreg[X] - vreg[Y];
	break;
      }
      case 0x6:{
	//right shift
	//maybe put a flag or smth that can be set to enable
	//compatibility for both COSMAC VP and CHIP-48 roms

	if (COSMAC_VP_MODE){}
	else{
	  if (DEBUG_MODE)
	    printf("shifting %x right \n",vreg[X]);

	std::bitset<8> temp = vreg[X];
	vreg[X] = vreg[X] >> 1;
	vreg[0xF] = temp[7];

	if (DEBUG_MODE)
	  printf("result: %x, %b\n", vreg[X], vreg[15]);
	}
	break;
      }
      case 0x7:{
	//VY - VX
	if (vreg[Y] > vreg[X])
	  vreg[0xF] = 1;
	else
	  vreg[0xF] = 0;
	
	vreg[X] = vreg[Y] - vreg[X];
	break;
      }
      case 0xe:{
	//left shift
	std::bitset<8> temp = vreg[X];
	vreg[X] = vreg[X] << 1;
	vreg[0xF] = temp[0];
	break;}
      }
      break;
    }
    case 0x9:{
      //skip if VX and VY are unequal
      if (vreg[X] != vreg[Y])
	PC += 2;
      break;
    }
    case 0xa:{
      I = (NNN - 1);
      //printf("set I to %x\n", I);
      break;
    }
    case 0xb:{
      //jump to NNN + V0
      //might want to implement configuration here
      PC = (NNN - 1) + vreg[0];
      break;
    }
    case 0xc:{ //generate random number
      unsigned char r = rand() % 255;
      vreg[X] = r & NN;
      break;
    }
    case 0xd:{
      //display/draw

      vreg[0xF] = 0;
            
      unsigned char cY = (vreg[Y] % 32);
      unsigned char cX = (vreg[X] % 64);
      
      for (size_t row = 0; row < N; ++row){
	//printf("x: %i y: %i\n", cX, cY);
	
	std::bitset<8> data = mem[I + row];

	//std::cout << data << "\n";
	size_t bit = 8;
	//for (size_t bit = 0; bit < 8; ++bit){
	while (bit >= 0){
	  if ((!display[cX][cY]) && (data[bit]))
	    display[cX][cY] = 1;

	  else if (display[cX][cY] && data[bit]){
	    display[cX][cY] = 0;
	    vreg[0xF] = 1;
	  }
	  if (cX > 63)
	    break;
	  cX += 1;
	  bit -= 1;
	  }
	
	if (cY > 31)
	  break;
	
	cY += 1;
	cX = (vreg[X] % 64);
      }
      
      //
	  // printf("%b ", display[coordX+bit][coordY+row]);
	//std::cout << data << " \n";
      
      break;
    }
    case 0xe:{//skip if key
      if (vreg[X] > 0xf){
	printf("invalid keycode!\n");
	break;
      }
      
      switch(NN){
      case 0x9e:{
	if (pressedKey == vreg[X]){
	  PC += 2;
	}
	break;}
      case 0xA1:{
	if (pressedKey != vreg[X]){
	  PC += 2;
	}
	break;}
      }
      break;
    }
    case 0xf:{//control timers
      switch(NN){
      case 0x07:{
	vreg[X] = delayTimer;
	break;}
      case 0x15:{
	delayTimer = vreg[X];
	break;}
      case 0x18:{
	soundTimer = vreg[X];
	break;}
      case 0x1e:{//add to index
	if ((I + vreg[X]) >= 0x1000)
	  vreg[0xF] = 1;
	I = I + vreg[X];
	break;}
      case 0x0A:{//delay until get key
	if ((pressedKey < 0) && (pressedKey < 0xf))
	  PC -= 2;
	else{//make note for how on the COSMAC VIP key had to be pressed
	  //then released
	  vreg[X] = pressedKey;
	}
	break;}
      case 0x29:{//get font character
	I = FONT_ADDRESSES[vreg[X]];
	break;}
      case 0x33:{//binary to decimal conversion
	
	mem[I] = vreg[X] / 100;
	mem[I+1] = (vreg[X] % 100)/10;
	mem[I+2] = (vreg[X] % 10);

	if (DEBUG_MODE){
	  printf("convert %i to individual decimals: %x, %x, %x\n", vreg[X], mem[I], mem[I+1], mem[I+2]);
	}
	break;}
      case 0x55:{//store memory (store value of registers from v0 to vX)
	if (DEBUG_MODE)
	  printf("store memory from registers V0 to V%x\n", X);

	int count = 0;
	while (count < X+1){
	  if (DEBUG_MODE){
	    printf("set address %x to %x\n", mem[I+count], vreg[count]);
	  }
	  
	  mem[I + count] = vreg[count];
	  ++count;
	}
	break;}
      case 0x65:{//load memory 

	int count = 0;
	while(count < X+1){

	  if (DEBUG_MODE){
	    printf("set V%x to %x\n", count, mem[I+count]);
	  }
	  
	  vreg[count] = mem[I + count];
	  ++count;
	}
	break;}
     }
    }
    }  
    
    //rendering
    //off color = polynesian blue
    SDL_SetRenderDrawColor(renderer, OFF_R, OFF_G, OFF_B, 0xFF);
    
    SDL_RenderClear(renderer);

    for (size_t w = 0; w < 63; ++w){
      for (size_t h = 0; h < 31; ++h){
	//printf("%b ", display[w][h]);
	if (display[w][h]){
	  drawPixel(w*10, h*10, renderer);
	}
      }
    }

    if (delayTimer > 0){
      delayTimer -= 1;
    }

    if (soundTimer > 0){
      //implement beeping noise
      soundTimer -= 1;
    }
    
    SDL_RenderPresent(renderer);
    SDL_Delay(10);
  }
  close(window, renderer);
  f.close();
  
  return 0;
}

