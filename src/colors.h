typedef struct _color_t   color_t;

struct _color_t {
  char* suffix;
  char* color;
  GdkGC* gc;
};

void color_save(GList* colors, char* name);
int color_load(char* name);
void color_create_default();
color_t* color_search(char* filename);
