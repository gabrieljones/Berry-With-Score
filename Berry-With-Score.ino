/*
 *  Berry w/ Score Hinting
 *  by VV Studio
 *  at IndieCade East 2018 Game Jam
 *  Lead development by Jonathan Bobrow, Move38 Inc.
 *  original game by ViVi and Vanilla
 *
 *  Remixed 11.21.2020
 *  by Jonathan Bobrow
 *  
 *  Rules: https://github.com/Move38/Berry/blob/master/README.md
 *
 *  --------------------
 *  Blinks by Move38
 *  Brought to life via Kickstarter 2018
 *
 *  @madewithblinks
 *  www.move38.com
 *  --------------------
 */

Color colors[] = { BLUE, RED, YELLOW };
byte currentColorIndex = 0;
byte faceIndex = 0;
byte faceStartIndex = 0;
//byte localScore = 0;

bool isWaiting = false;

#define FACE_DURATION 60
#define WAIT_DURATION 2000

Timer faceTimer;

// SYNCHRONY
Timer syncTimer;
#define PERIOD_DURATION 3000
#define BUFFER_DURATION 200
byte neighborState[6];
byte syncVal = 0;

Timer scoreTimer;
#define SCORE_PULSE_DURATION 300

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

  if ( buttonSingleClicked() ) {

    currentColorIndex++;

    if (currentColorIndex >= COUNT_OF(colors)) {
      currentColorIndex = 0;
    }

  }

  if ( syncTimer.getRemaining() >= 2000 && syncTimer.getRemaining() <= 2370 ) {
    isWaiting = false;
    if ( faceTimer.isExpired() ) {
      faceTimer.set( FACE_DURATION );
      faceIndex++;

      if (faceIndex >= 7) {
        faceIndex = 0;
        
        // shift the starting point
        faceStartIndex++;
        if (faceStartIndex >= 6) {
          faceStartIndex = 0;
        }
      }
    }
  }
  else {
    isWaiting = true;
  }


  // sync the signaling for points
  syncLoop();

  // display color
  setColor( colors[currentColorIndex] );

  // show locked sides
//  if (isPositionLocked()) {
//    // show the state of locked animation on all faces
//    byte bri = 153 + (sin8_C((millis() / 6) % 255)*2)/5;
//    setColor(dim(colors[currentColorIndex], bri));
//  }

  // show next color
  if (!isWaiting) {
    byte nextColorIndex = (currentColorIndex + 1) % 3;
    byte face = (faceStartIndex + faceIndex - 1) % FACE_COUNT;
    setFaceColor( face, colors[nextColorIndex] );
  }

  // display score
  displayScore();


  // Communications for scoring
  byte localScore = calculateLocalPointValue();
  byte data = (syncVal << 5) + (localScore << 2) + currentColorIndex;
  setValueSentOnAllFaces(data);
}

/*
 * 
 */
void displayScore() {

  byte score = calculateGlobalPointValue();

  // if the score is 1 point, dim in sync on the faces connected to same colors
  // if the score is 2 points, dim in sync on the faces connected to same colors
  // if the score is 2 bowtie style, dim on the middle piece?
  // if the score is 3 points, dim on the one surrounded by 4 pieces
  if(score == 3) {
    if((syncTimer.getRemaining() >= 0 && syncTimer.getRemaining() <= 300) ||
       (syncTimer.getRemaining() >= 600 && syncTimer.getRemaining() <= 900) ||
       (syncTimer.getRemaining() >= 1200 && syncTimer.getRemaining() <= 1500))  {
  
      FOREACH_FACE(f) {
        if(!isValueReceivedOnFaceExpired(f)) {
          byte neighborData = getLastValueReceivedOnFace(f);
          
          if(isMyColor(neighborData)) {
            // dim on this face
            setColorOnFace(OFF,f);
          }
        }
      }
      
    }
  }
  else if(score == 2) {
    if((syncTimer.getRemaining() >= 0 && syncTimer.getRemaining() <= 300) ||
       (syncTimer.getRemaining() >= 600 && syncTimer.getRemaining() <= 900))  {
  
      FOREACH_FACE(f) {
        if(!isValueReceivedOnFaceExpired(f)) {
          byte neighborData = getLastValueReceivedOnFace(f);
          
          if(isMyColor(neighborData)) {
            // dim on this face
            setColorOnFace(OFF,f);
          }
        }
      }
      
    }
  }
  else if(score == 1) {
    if((syncTimer.getRemaining() >= 0 && syncTimer.getRemaining() <= 300))  {
  
      FOREACH_FACE(f) {
        if(!isValueReceivedOnFaceExpired(f)) {
          byte neighborData = getLastValueReceivedOnFace(f);
          
          if(isMyColor(neighborData)) {
            // dim on this face
            setColorOnFace(OFF,f);
          }
        }
      }
      
    }
  }

}


byte calculateGlobalPointValue() {
  // check neighbors for highest score, if my score is the highest, keep that
  byte highScore = 0;
  
  FOREACH_FACE(f) {
    if(!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      
      if(isMyColor(neighborData)) {
        byte score = getNeighborScore(neighborData);

        if(score > highScore ) {
          highScore = score;
        }
      }
    }
  }
  byte localScore = calculateLocalPointValue();
  if( localScore > highScore ) {
    highScore = localScore;
  }

  return highScore;
}

/*
 * Return the value
 */
byte calculateLocalPointValue() {
  byte score = 0;
  
  // get neighbor pattern of my color
  bool neighborIsMyColor[] = {0,0,0,0,0,0};
  FOREACH_FACE(f) {
    if(!isValueReceivedOnFaceExpired(f)) {
      if(isMyColor(getLastValueReceivedOnFace(f))) {
        neighborIsMyColor[f] = true;
      }
    }
  }

  bool bowtie[6] = {1,1,0,1,1,0};
  bool triangle[6] = {1,1,0,0,0,0};
  bool three[6] = {1,1,1,1,0,0};
  bool diamond[6] = {1,1,1,0,0,0};
  bool flower[6] = {1,1,1,1,1,0};
  
  // if 2 of my neighbors are my color, 1 point
  if(isThisPatternPresent( triangle, neighborIsMyColor)) {
    score = 1;
  }
  // if 3 of my adjacent neighbors are my color, 2 points
  if(isThisPatternPresent( diamond, neighborIsMyColor)) {
    score = 2;
  }
  // if 4 of my neighbors (2 + 2) are my color, 2 points
  if(isThisPatternPresent( bowtie, neighborIsMyColor)) {
    score = 2;
  }
  // if 4 of my adjacent neighbors are my color, 3 points
  if(isThisPatternPresent( three, neighborIsMyColor)) {
    score = 3;
  }
  // if 5 of my adjacent neighbors are my color, 10 points WIN
  if(isThisPatternPresent( flower, neighborIsMyColor)) {
    score = 6;
  }
  
  return score;
}

/*
 * Get the shared Sync Value
 */
byte getSyncVal(byte data) {
  return (data >> 5) & 1;  
}

/*
 * Extract my neighbor's score from the comms data
 */
byte getNeighborScore(byte data) {
  return (data >> 2) & 7;
}

/*
 * Determine if the neighboring Blink is my color
 */
bool isMyColor(byte data) {
  return (data & 3) == currentColorIndex;
}

/*
 * Determine if a piece is in a locked position
 * If a piece is in this kind of location, determined 
 * by the pattern of surrounding Blinks, it cannot
 * legally be moved.
 */
bool isPositionLocked() {
  // based on the arrangement of neighbors, am I locked...
  bool neighborPattern[6];
  bool lockedA[6] = {1, 0, 1, 0, 1, 0};
  bool lockedB[6] = {1, 0, 1, 0, 0, 0};

  FOREACH_FACE(f) {
    neighborPattern[f] = !isValueReceivedOnFaceExpired(f);
  }

  // neighbors across from each other
  for (byte i = 0; i < 3; i++) {
    if (neighborPattern[i] && neighborPattern[i + 3]) {
      return true;
    }
  }

  // special case lock patterns
  if ( isThisPatternPresent(lockedA, neighborPattern)) {
    return true;
  }
  if ( isThisPatternPresent(lockedB, neighborPattern)) {
    return true;
  }

  return false;
}

// check to see if pattern is in the array
// return true if the pattern is in fact in the array
// pattern is always 6 bools
// source is always 12 bools (2 x 6 bools)
bool isThisPatternPresent( bool pat[], bool source[]) {

  // first double the source to be cyclical
  bool source_double[12];

  for (byte i = 0; i < 12; i++) {
    source_double[i] = source[i % 6];
  }

  // then find the pattern
  byte pat_index = 0;

  for (byte i = 0; i < 12; i++) {
    if (source_double[i] == pat[pat_index]) {
      // increment index
      pat_index++;

      if ( pat_index == 6 ) {
        // found the entire pattern
        return true;
      }
    }
    else {
      // set index back to 0
      pat_index = 0;
    }
  }

  return false;
}

/*
   Keep ourselves on the same time loop as our neighbors
   if a neighbor passed go,
   we want to pass go as well
   (if we didn't just pass go)
   ... or collect $200
*/
void syncLoop() {

  bool didNeighborChange = false;

  // look at our neighbors to determine if one of them passed go (changed value)
  // note: absent neighbors changing to not absent don't count
  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f)) {
      neighborState[f] = 2; // this is an absent neighbor
    }
    else {
      byte data = getLastValueReceivedOnFace(f);
      if (neighborState[f] != 2) {  // wasn't absent
        if (getSyncVal(data) != neighborState[f]) { // passed go (changed value)
          didNeighborChange = true;
        }
      }

      neighborState[f] = getSyncVal(data);  // update our record of state now that we've check it
    }
  }

  // if our neighbor passed go and we haven't done so within the buffer period, catch up and pass go as well
  // if we are due to pass go, i.e. timer expired, do so
  if ( (didNeighborChange && syncTimer.getRemaining() < PERIOD_DURATION - BUFFER_DURATION)
       || syncTimer.isExpired()
     ) {

    syncTimer.set(PERIOD_DURATION); // aim to pass go in the defined duration
    syncVal = !syncVal; // change our value everytime we pass go
  }
}
