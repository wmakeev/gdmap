#define P_SIMPLE   0
#define P_3D       1
#define P_GRADIENT 2
#define P_CUSHION  3

#define BORDER   1

typedef struct _global_t   global_t;

struct _global_t {
  GtkWidget* win;

  dir_tree_t* current_tree;
  GList* trees;
  dir_tree_t* mark;
  dir_tree_t* mark2;
  char* gdmap_home;
  GList* colors;
  int expose_timer;
  int paint_mode;
  int max_depth;
};

extern global_t global;

char* size_format(gint64 size);
void create_dir(char* dir);
char* arg(char* data, int last);
void global_init();
int expose_timeout_cb(gpointer data);
