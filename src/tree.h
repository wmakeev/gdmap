typedef struct _dir_tree_t       dir_tree_t;
typedef struct _geometry_t       geometry_t;

struct _dir_tree_t {
  char* name;
  gint64 size;
  long dir_size;
  GList* entries;
  dir_tree_t* parent;
  double geo[2][2];
  int depth;

  int width;
  int height;
  unsigned char* buf;
};

dir_tree_t* dir_tree_search(char* name);
void dir_tree_show(char* name);
void dir_tree_display(dir_tree_t* tree);
dir_tree_t* dir_tree_search_file(dir_tree_t* tree, int x, int y);
void dir_tree_mark(dir_tree_t* sub_tree);
GtkWidget* create_right_popup(dir_tree_t* current);
char* dir_tree_full_name(dir_tree_t* tree);
