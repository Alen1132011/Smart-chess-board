#include <Arduino.h>
#include <FastLED.h>

void calibratesensors();
void initializeboard();
bool isenpassantpossible(int from, int to);

const int ledPin = 3;
const int latchPin = 2;
const int clockPin = 4;
const int dataPin = 5;
const int numLEDs = 64;

CRGB leds[numLEDs];
bool sensorstate[numLEDs];
bool lastsensorstate[numLEDs];
bool sensorbaseline[numLEDs];
bool sensorinverted = false;

bool whitekinghasmoved = false;
bool whiterookkingsidehasmoved = false;
bool whiterookqueensidehasmoved = false;
bool blackkinghasmoved = false;
bool blackrookkingsidehasmoved = false;
bool blackrookqueensidehasmoved = false;
int enpassanttarget = -1;

enum PieceType {
  EMPTY,
  WP,
  WN,
  WB,
  WR,
  WQ,
  WK,
  BP,
  BN,
  BB,
  BR,
  BQ,
  BK
};

PieceType board[64];
bool whiteTurn = true;
int selectedsquare = -1;

PieceType getpieceat(int square) {
  if (square < 0 || square >= 64) return EMPTY;
  return board[square];
}

bool iswhitepiece(PieceType piece) {
  return piece == WP || piece == WN || piece == WB ||
         piece == WR || piece == WQ || piece == WK;
}

bool isblackpiece(PieceType piece) {
  return piece == BP || piece == BN || piece == BB ||
         piece == BR || piece == BQ || piece == BK;
}

bool isoccupied(int square) {
  return getpieceat(square) != EMPTY;
}

bool readsensorvalue(int index) {
  bool raw = (digitalRead(dataPin) == HIGH);
  return sensorinverted ? !raw : raw;
}

bool ispathclear(int from, int to) {
  int dx = (to % 8) - (from % 8);
  int dy = (to / 8) - (from / 8);
  int stepx = 0;
  int stepy = 0;

  if (dx != 0) stepx = dx > 0 ? 1 : -1;
  if (dy != 0) stepy = dy > 0 ? 1 : -1;

  int steps = max(abs(dx), abs(dy));

  for (int i = 1; i < steps; i++) {
    int x = (from % 8) + stepx * i;
    int y = (from / 8) + stepy * i;
    int square = y * 8 + x;

    if (isoccupied(square)) return false;
  }

  return true;
}

bool isknightmove(int from, int to) {
  int dx = abs((to % 8) - (from % 8));
  int dy = abs((to / 8) - (from / 8));

  return (dx == 2 && dy == 1) || (dx == 1 && dy == 2);
}

bool ispawnmove(int from, int to, bool iswhite) {
  int dx = (to % 8) - (from % 8);
  int dy = (to / 8) - (from / 8);

  if (iswhite) {
    if (dx == 0) {
      if (dy == -1 && !isoccupied(to)) return true;

      if (dy == -2 &&
          from / 8 == 6 &&
          !isoccupied(to) &&
          !isoccupied(from - 8)) {
        return true;
      }
    }

    if (abs(dx) == 1 && dy == -1) {
      if (isoccupied(to) && isblackpiece(getpieceat(to))) return true;
      if (isenpassantpossible(from, to)) return true;
    }
  } else {
    if (dx == 0) {
      if (dy == 1 && !isoccupied(to)) return true;

      if (dy == 2 &&
          from / 8 == 1 &&
          !isoccupied(to) &&
          !isoccupied(from + 8)) {
        return true;
      }
    }

    if (abs(dx) == 1 && dy == 1) {
      if (isoccupied(to) && iswhitepiece(getpieceat(to))) return true;
      if (isenpassantpossible(from, to)) return true;
    }
  }

  return false;
}

bool iskingmove(int from, int to) {
  int dx = abs((to % 8) - (from % 8));
  int dy = abs((to / 8) - (from / 8));

  return dx <= 1 && dy <= 1;
}

bool issquareunderattack(int square, bool bywhite) {
  for (int i = 0; i < 64; i++) {
    PieceType piece = getpieceat(i);

    if (piece == EMPTY) continue;

    bool pieceiswhite = iswhitepiece(piece);

    if (pieceiswhite != bywhite) continue;

    switch (piece) {
      case EMPTY:
        break;

      case WP:
        if (abs((i % 8) - (square % 8)) == 1 &&
            (square / 8) == (i / 8) - 1) {
          return true;
        }
        break;

      case BP:
        if (abs((i % 8) - (square % 8)) == 1 &&
            (square / 8) == (i / 8) + 1) {
          return true;
        }
        break;

      case WN:
      case BN:
        if (isknightmove(i, square)) return true;
        break;

      case WB:
      case BB:
        if (abs((square % 8) - (i % 8)) ==
                abs((square / 8) - (i / 8)) &&
            ispathclear(i, square)) {
          return true;
        }
        break;

      case WR:
      case BR:
        if (((square % 8) == (i % 8) ||
             (square / 8) == (i / 8)) &&
            ispathclear(i, square)) {
          return true;
        }
        break;

      case WQ:
      case BQ:
        if ((((square % 8) == (i % 8) ||
              (square / 8) == (i / 8)) ||
             abs((square % 8) - (i % 8)) ==
                 abs((square / 8) - (i / 8))) &&
            ispathclear(i, square)) {
          return true;
        }
        break;

      case WK:
      case BK:
        if (iskingmove(i, square)) return true;
        break;
    }
  }

  return false;
}

bool isincheck(bool whiteTurn) {
  int kingsquare = -1;

  for (int i = 0; i < 64; i++) {
    PieceType piece = getpieceat(i);

    if (piece == (whiteTurn ? WK : BK)) {
      kingsquare = i;
      break;
    }
  }

  if (kingsquare == -1) return false;

  return issquareunderattack(kingsquare, !whiteTurn);
}

bool isenpassantpossible(int from, int to) {
  if (enpassanttarget == -1) return false;

  PieceType piece = getpieceat(from);
  bool iswhite = iswhitepiece(piece);

  if (piece != WP && piece != BP) return false;

  if (abs((from % 8) - (to % 8)) != 1) return false;

  int pawardirection = iswhite ? -1 : 1;

  if ((to / 8) - (from / 8) != pawardirection) return false;

  if (to != enpassanttarget) return false;

  int passedpawnsquare =
      (enpassanttarget / 8) * 8 + (from % 8);

  PieceType passedpawn = getpieceat(passedpawnsquare);

  if (iswhite) {
    return passedpawn == BP;
  }

  return passedpawn == WP;
}

bool wouldmovecausecheck(int from, int to, bool whiteTurn) {
  PieceType captured = board[to];
  PieceType mover = board[from];

  bool enpassant = isenpassantpossible(from, to);
  int capturedpawnsquare = -1;
  PieceType enpassantcaptured = EMPTY;

  if (enpassant) {
    capturedpawnsquare =
        (enpassanttarget / 8) * 8 + (from % 8);

    enpassantcaptured = board[capturedpawnsquare];
    board[capturedpawnsquare] = EMPTY;
  }

  board[to] = mover;
  board[from] = EMPTY;

  bool incheck = isincheck(whiteTurn);

  board[from] = mover;
  board[to] = captured;

  if (enpassant) {
    board[capturedpawnsquare] = enpassantcaptured;
  }

  return incheck;
}

bool iscastlingpossible(
    int kingfrom,
    int rookfrom,
    int kingto,
    bool whiteside) {

  if (whiteside) {
    if (whitekinghasmoved) return false;

    if (rookfrom == 0 && whiterookqueensidehasmoved)
      return false;

    if (rookfrom == 7 && whiterookkingsidehasmoved)
      return false;

    if (board[rookfrom] != WR) return false;
  } else {
    if (blackkinghasmoved) return false;

    if (rookfrom == 0 && blackrookqueensidehasmoved)
      return false;

    if (rookfrom == 7 && blackrookkingsidehasmoved)
      return false;

    if (board[rookfrom] != BR) return false;
  }

  if (isincheck(whiteside)) return false;

  int step = (kingto > kingfrom) ? 1 : -1;

  for (int sq = kingfrom + step; sq != kingto; sq += step) {
    if (isoccupied(sq)) return false;

    if (wouldmovecausecheck(kingfrom, sq, whiteside))
      return false;
  }

  return !wouldmovecausecheck(kingfrom, kingto, whiteside);
}

bool ispawnpromotion(int from, int to, PieceType piece) {
  if (piece != WP && piece != BP) return false;

  int torank = to / 8;

  return (iswhitepiece(piece) && torank == 0) ||
         (!iswhitepiece(piece) && torank == 7);
}

bool islegalmove(int from, int to) {
  if (from < 0 || from >= 64 ||
      to < 0 || to >= 64) {
    return false;
  }

  if (from == to) return false;

  PieceType piece = getpieceat(from);

  if (piece == EMPTY) return false;

  bool pieceiswhite = iswhitepiece(piece);

  if ((whiteTurn && !pieceiswhite) ||
      (!whiteTurn && pieceiswhite)) {
    return false;
  }

  if (isoccupied(to) &&
      iswhitepiece(getpieceat(to)) == pieceiswhite) {
    return false;
  }

  switch (piece) {
    case EMPTY:
      return false;

    case WP:
      return ispawnmove(from, to, true) &&
             !wouldmovecausecheck(from, to, true);

    case BP:
      return ispawnmove(from, to, false) &&
             !wouldmovecausecheck(from, to, false);

    case WN:
    case BN:
      return isknightmove(from, to) &&
             !wouldmovecausecheck(from, to, whiteTurn);

    case WB:
    case BB:
      if (!ispathclear(from, to)) return false;

      if (abs((to % 8) - (from % 8)) !=
          abs((to / 8) - (from / 8))) {
        return false;
      }

      return !wouldmovecausecheck(from, to, whiteTurn);

    case WR:
    case BR:
      if (!ispathclear(from, to)) return false;

      if ((from % 8) != (to % 8) &&
          (from / 8) != (to / 8)) {
        return false;
      }

      return !wouldmovecausecheck(from, to, whiteTurn);

    case WQ:
    case BQ: {
      if (!ispathclear(from, to)) return false;

      bool diagonal =
          abs((to % 8) - (from % 8)) ==
          abs((to / 8) - (from / 8));

      bool straight =
          (from % 8) == (to % 8) ||
          (from / 8) == (to / 8);

      if (!(diagonal || straight)) return false;

      return !wouldmovecausecheck(from, to, whiteTurn);
    }

    case WK: {
      int dx = abs((to % 8) - (from % 8));
      int dy = abs((to / 8) - (from / 8));

      if (dx == 2 && dy == 0) {
        if (!whiteTurn) return false;

        return iscastlingpossible(
            from,
            (to > from) ? 7 : 0,
            to,
            true
        );
      }

      if (!iskingmove(from, to)) return false;

      return !wouldmovecausecheck(from, to, whiteTurn);
    }

    case BK: {
      int dx = abs((to % 8) - (from % 8));
      int dy = abs((to / 8) - (from / 8));

      if (dx == 2 && dy == 0) {
        if (whiteTurn) return false;

        return iscastlingpossible(
            from,
            (to > from) ? 7 : 0,
            to,
            false
        );
      }

      if (!iskingmove(from, to)) return false;

      return !wouldmovecausecheck(from, to, whiteTurn);
    }

    default:
      return false;
  }
}

void makemove(int from, int to) {
  if (!islegalmove(from, to)) return;

  PieceType piece = getpieceat(from);
  bool iswhite = iswhitepiece(piece);

  bool enpassant = isenpassantpossible(from, to);

  int capturedpawnsquare = -1;

  if (enpassant) {
    capturedpawnsquare =
        (enpassanttarget / 8) * 8 + (from % 8);
  }

  if (iswhitepiece(piece)) {
    if (piece == WK) {
      whitekinghasmoved = true;
    }

    if (piece == WR) {
      if (from == 0)
        whiterookqueensidehasmoved = true;

      if (from == 7)
        whiterookkingsidehasmoved = true;
    }
  } else {
    if (piece == BK) {
      blackkinghasmoved = true;
    }

    if (piece == BR) {
      if (from == 0)
        blackrookqueensidehasmoved = true;

      if (from == 7)
        blackrookkingsidehasmoved = true;
    }
  }

  if (board[to] == WR) {
    if (to == 0)
      whiterookqueensidehasmoved = true;

    if (to == 7)
      whiterookkingsidehasmoved = true;
  }

  if (board[to] == BR) {
    if (to == 0)
      blackrookqueensidehasmoved = true;

    if (to == 7)
      blackrookkingsidehasmoved = true;
  }

  board[to] = piece;
  board[from] = EMPTY;

  if (ispawnpromotion(from, to, piece)) {
    board[to] = iswhite ? WQ : BQ;
  }

  if (enpassant && capturedpawnsquare >= 0) {
    board[capturedpawnsquare] = EMPTY;
  }

  if (piece == WP || piece == BP) {
    if (abs((to / 8) - (from / 8)) == 2) {
      enpassanttarget = (from + to) / 2;
    } else {
      enpassanttarget = -1;
    }
  } else {
    enpassanttarget = -1;
  }

  if (piece == WK &&
      abs((to % 8) - (from % 8)) == 2) {

    if (to > from) {
      board[5] = board[7];
      board[7] = EMPTY;
    } else {
      board[3] = board[0];
      board[0] = EMPTY;
    }
  }

  if (piece == BK &&
      abs((to % 8) - (from % 8)) == 2) {

    if (to > from) {
      board[61] = board[63];
      board[63] = EMPTY;
    } else {
      board[59] = board[56];
      board[56] = EMPTY;
    }
  }

  whiteTurn = !whiteTurn;
}

void readsensors() {
  digitalWrite(latchPin, LOW);
  delayMicroseconds(5);
  digitalWrite(latchPin, HIGH);

  for (int i = 0; i < numLEDs; i++) {
    sensorstate[i] = readsensorvalue(i);

    digitalWrite(clockPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(clockPin, LOW);
  }
}

void updateleds() {
  for (int i = 0; i < numLEDs; i++) {
    leds[i] = CRGB::Black;
  }

  for (int i = 0; i < numLEDs; i++) {
    PieceType piece = getpieceat(i);

    if (piece == EMPTY) continue;

    if (iswhitepiece(piece)) {
      leds[i] = CRGB::White;
    } else {
      leds[i] = CRGB::Blue;
    }
  }

  if (selectedsquare != -1) {
    leds[selectedsquare] = CRGB::Yellow;
  }
}

void checkformove() {
  static bool piecelifted = false;
  static int liftsquare = -1;

  readsensors();

  bool changed = false;

  for (int i = 0; i < numLEDs; i++) {
    if (sensorstate[i] != lastsensorstate[i]) {
      changed = true;
      lastsensorstate[i] = sensorstate[i];
    }
  }

  if (!changed) return;

  if (!piecelifted) {
    for (int i = 0; i < numLEDs; i++) {
      if (!sensorstate[i] &&
          lastsensorstate[i] &&
          isoccupied(i)) {

        piecelifted = true;
        liftsquare = i;
        selectedsquare = i;

        break;
      }
    }
  } else {
    for (int i = 0; i < numLEDs; i++) {
      if (sensorstate[i] &&
          !lastsensorstate[i]) {

        if (islegalmove(liftsquare, i)) {
          makemove(liftsquare, i);
        }

        piecelifted = false;
        liftsquare = -1;
        selectedsquare = -1;

        break;
      }
    }

    if (piecelifted) {
      for (int i = 0; i < numLEDs; i++) {
        if (sensorstate[i] &&
            liftsquare == i) {

          piecelifted = false;
          liftsquare = -1;
          selectedsquare = -1;

          break;
        }
      }
    }
  }
}

bool ischeckmate() {
  if (!isincheck(whiteTurn)) return false;

  for (int from = 0; from < numLEDs; from++) {
    for (int to = 0; to < numLEDs; to++) {
      if (islegalmove(from, to)) {
        return false;
      }
    }
  }

  return true;
}

bool isstalemate() {
  if (isincheck(whiteTurn)) return false;

  for (int from = 0; from < numLEDs; from++) {
    for (int to = 0; to < numLEDs; to++) {
      if (islegalmove(from, to)) {
        return false;
      }
    }
  }

  return true;
}

void calibratesensors() {
  bool samples[10][64];

  for (int sample = 0; sample < 10; sample++) {
    digitalWrite(latchPin, LOW);
    delayMicroseconds(5);
    digitalWrite(latchPin, HIGH);

    for (int i = 0; i < numLEDs; i++) {
      samples[sample][i] =
          (digitalRead(dataPin) == HIGH);

      digitalWrite(clockPin, HIGH);
      delayMicroseconds(5);
      digitalWrite(clockPin, LOW);
    }

    delay(10);
  }

  for (int i = 0; i < 64; i++) {
    int highcount = 0;

    for (int sample = 0; sample < 10; sample++) {
      if (samples[sample][i]) highcount++;
    }

    sensorbaseline[i] = (highcount >= 5);
  }

  int baselinehigh = 0;

  for (int i = 0; i < 64; i++) {
    if (sensorbaseline[i]) baselinehigh++;
  }

  sensorinverted = (baselinehigh > 32);
}

void initializeboard() {
  for (int i = 0; i < 64; i++) {
    board[i] = EMPTY;
    lastsensorstate[i] = sensorbaseline[i];
  }

  board[0] = BR;
  board[1] = BN;
  board[2] = BB;
  board[3] = BQ;
  board[4] = BK;
  board[5] = BB;
  board[6] = BN;
  board[7] = BR;

  board[8] = BP;
  board[9] = BP;
  board[10] = BP;
  board[11] = BP;
  board[12] = BP;
  board[13] = BP;
  board[14] = BP;
  board[15] = BP;

  board[48] = WP;
  board[49] = WP;
  board[50] = WP;
  board[51] = WP;
  board[52] = WP;
  board[53] = WP;
  board[54] = WP;
  board[55] = WP;

  board[56] = WR;
  board[57] = WN;
  board[58] = WB;
  board[59] = WQ;
  board[60] = WK;
  board[61] = WB;
  board[62] = WN;
  board[63] = WR;

  whitekinghasmoved = false;
  whiterookkingsidehasmoved = false;
  whiterookqueensidehasmoved = false;

  blackkinghasmoved = false;
  blackrookkingsidehasmoved = false;
  blackrookqueensidehasmoved = false;

  enpassanttarget = -1;
  whiteTurn = true;
  selectedsquare = -1;
}

void setup() {
  FastLED.addLeds<WS2812B, ledPin, GRB>(
      leds,
      numLEDs
  );

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, INPUT);

  Serial.begin(115200);

  FastLED.setBrightness(50);
  FastLED.clear();
  FastLED.show();

  calibratesensors();
  initializeboard();
}

void loop() {
  checkformove();
  updateleds();
  FastLED.show();

  delay(50);
}