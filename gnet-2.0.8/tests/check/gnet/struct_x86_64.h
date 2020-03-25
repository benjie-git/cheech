
GNetCheckABIStruct list[] = {
  {"GConnEvent", sizeof (GConnEvent), 24},
  {"GConnHttpEvent", sizeof (GConnHttpEvent), 48},
  {"GConnHttpEventResolved", sizeof (GConnHttpEventResolved), 88},
  {"GConnHttpEventRedirect", sizeof (GConnHttpEventRedirect), 104},
  {"GConnHttpEventResponse", sizeof (GConnHttpEventResponse), 136},
  {"GConnHttpEventData", sizeof (GConnHttpEventData), 144},
  {"GConnHttpEventError", sizeof (GConnHttpEventError), 96},
  {"GServer", sizeof (GServer), 48},
  {"GURI", sizeof (GURI), 56},
  {NULL, 0, 0}
};
