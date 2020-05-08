/****************************************************************************

  Header file for template service

 ****************************************************************************/

#ifndef ServKeyboard_H
#define ServKeyboard_H

#include "ES_Event.h"
#include <stdbool.h>
#include <stdint.h>

// Public Function Prototypes
bool InitKeyboardService(uint8_t Priority);
bool PostKeyboardService(ES_Event_t ThisEvent);
ES_Event_t RunKeyboardService(ES_Event_t ThisEvent);

// Event checkers
bool EventCheckerKeyBoard(); 

#endif /* ServKeyboard_H */

