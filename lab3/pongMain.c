 
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6

char SW1, SW2, SW3, SW4; //switches on the board
char counter1, counter2 = 0; //keeps track of the scores

/** arrays to store the scores of the players */
char scoreTop[] = {"0"}; 
char scoreBottom[] = {"0"};

/*rectangles for the paddles */
AbRect rectPlayers = {abRectGetBounds, abRectCheck, {13,2}}; 

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 15}
};

/* center line in the field */
AbRect centerLine = {abRectGetBounds, abRectCheck, {screenWidth/2 - 10, 1}};

Layer centerLayer = {
  (AbShape *)&centerLine,
  {screenWidth/2, screenHeight/2},
  {0,0}, {0,0},
  COLOR_WHITE,
  0,
};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &centerLayer
};

/* top paddle */
Layer topPlayer = {
  (AbShape *)&rectPlayers,
  {screenWidth/2, 20},
  {0,0}, {0,0},
  COLOR_RED,
  &fieldLayer
};

/* bottom paddle */
Layer bottomPlayer = {
  (AbShape *)&rectPlayers,
  {screenWidth/2, screenHeight - 20},
  {0,0}, {0,0},
  COLOR_GREEN,
  &topPlayer
};

Layer layer0 = {		
  (AbShape *)&circle2,
  {(screenWidth/2), (screenHeight/2)}, 
  {0,0}, {0,0},				    
  COLOR_BLUE,
  &bottomPlayer,
};

typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* linked list of moving layers */
MovLayer ml2 = {&bottomPlayer, {0,0}, 0};
MovLayer ml1 = {&topPlayer, {0,0}, &ml2};  
MovLayer ml0 = { &layer0, {3,3}, &ml1};

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  

void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;

  Region shapeBoundary;
  Region topPlayerBoundary; //get bounding rectangle of top paddle
  Region bottomPlayerBoundary; // get bounding rectangel of bottom paddle

  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */

  /* again define the bounding rectangle of all moving layers to make collision happen with the paddles */
  abShapeGetBounds(ml0.layer->abShape, &(ml0.layer->posNext), &shapeBoundary);
  abShapeGetBounds(ml1.layer->abShape, &(ml1.layer->posNext), &topPlayerBoundary);
  abShapeGetBounds(ml2.layer->abShape, &(ml2.layer->posNext), &bottomPlayerBoundary);

  /* collision with top player */
  if(shapeBoundary.topLeft.axes[1] < topPlayerBoundary.botRight.axes[1]) {
    if((shapeBoundary.topLeft.axes[0]+5 > topPlayerBoundary.topLeft.axes[0]) && 
       (shapeBoundary.botRight.axes[0]-5 < topPlayerBoundary.botRight.axes[0])) {
      int velocity = ml0.velocity.axes[1] = -ml0.velocity.axes[1];
      ml0.layer->posNext.axes[1] += (2*velocity);
    } 
    else {  //if ball touches top field layer
      counter1 ++;
      ml0.layer->posNext.axes[0] = screenWidth/2; //reset ball pos
      ml0.layer->posNext.axes[1] = screenHeight/2;
    }
    scoreTop[0] = '0' + counter1; //add score
    drawString5x7(screenWidth/2, screenHeight - 10, scoreTop, COLOR_GREEN, COLOR_BLACK); //display score
  }

  /* collision with bottom player*/
  if(shapeBoundary.botRight.axes[1] > bottomPlayerBoundary.topLeft.axes[1]) {
    if((shapeBoundary.topLeft.axes[0]+5 > bottomPlayerBoundary.topLeft.axes[0]) && 
       (shapeBoundary.botRight.axes[0]-5 < bottomPlayerBoundary.botRight.axes[0])) {
      int velocity = ml0.velocity.axes[1] = -ml0.velocity.axes[1];
      ml0.layer->posNext.axes[1] += (2*velocity);
    } 
    else {
      counter2 ++;
      ml0.layer->posNext.axes[0] = screenWidth/2; //reset ball pos
      ml0.layer->posNext.axes[1] = screenHeight/2;
    }
    scoreBottom[0] = '0' + counter2; //add score
    drawString5x7(screenWidth/2, 2, scoreBottom, COLOR_RED, COLOR_BLACK); //display score
  }
}

u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */

/*assigns the buttons to each player and direction */
void playerMov(){
  if (SW1)
    ml1.layer->posNext.axes[0] -=1;
  if (SW2)
    ml1.layer->posNext.axes[0] +=1;
  if (SW3)
    ml2.layer->posNext.axes[0] -=1;
  if (SW4)
    ml2.layer->posNext.axes[0] +=1;
}

/*start playing */
void play(){

    while (!redrawScreen) { 
      P1OUT &= ~GREEN_LED;    
      or_sr(0x10);	     
    }
    P1OUT |= GREEN_LED;    
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
}

void main()
{
  P1DIR |= GREEN_LED;			
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);


  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();    
  or_sr(0x8);
  
  while(1) {
    drawString5x7(2,2, "score:", COLOR_RED, COLOR_BLACK); //display score text
    drawString5x7(2,screenHeight -10, "score:", COLOR_GREEN, COLOR_BLACK);

    /* reset if player reaches 5 points */
    if (counter1 > 4 || counter2 > 4){
      counter1 = 0;
      counter2 = 0;
      scoreTop[0] = '0';
      scoreBottom[0] = '0';
      layerDraw(&layer0);
      drawString5x7(screenWidth/2, 2, scoreBottom, COLOR_RED, COLOR_BLACK);
      drawString5x7(screenWidth/2, screenHeight - 10, scoreTop, COLOR_GREEN, COLOR_BLACK);
      drawString5x7(2,2, "score:", COLOR_RED, COLOR_BLACK);
      drawString5x7(2,screenHeight -10, "score:", COLOR_GREEN, COLOR_BLACK);
    }    
  
    u_int switches = p2sw_read();
    SW1 = !(switches & BIT0);
    SW2 = !(switches & BIT1);
    SW3 = !(switches & BIT2);
    SW4 = !(switches & BIT3);
    
    drawString5x7(screenWidth/2, 2, scoreBottom, COLOR_RED, COLOR_BLACK);
    drawString5x7(screenWidth/2, screenHeight - 10, scoreTop, COLOR_GREEN, COLOR_BLACK);
    play();
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;	
  playerMov();	      
  count ++;
  if (count == 15) {
    mlAdvance(&ml0, &fieldFence);
    if (p2sw_read())
      redrawScreen = 1;
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
