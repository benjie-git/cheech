
GNetCheckABIStruct list[] = {
  {"GConnEvent", sizeof (GConnEvent), 12},
  {"GConnHttpEvent", sizeof (GConnHttpEvent), 24},
  {"GConnHttpEventResolved", sizeof (GConnHttpEventResolved), 44},
  {"GConnHttpEventRedirect", sizeof (GConnHttpEventRedirect), 56},
  {"GConnHttpEventResponse", sizeof (GConnHttpEventResponse), 68},
  {"GConnHttpEventData", sizeof (GConnHttpEventData), 80},
  {"GConnHttpEventError", sizeof (GConnHttpEventError), 48},
  {"GServer", sizeof (GServer), 24},
  {"GURI", sizeof (GURI), 28},
  {NULL, 0, 0}
};
