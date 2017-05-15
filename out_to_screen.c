#include <gtk/gtk.h>

void destroy(void) {
  gtk_main_quit();
}

int show_in_window (char * path) {
  GtkWidget* window;
  GtkWidget* image;
  int number;
  char ** argv;
  gtk_init (&number, &argv);


  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  image  = gtk_image_new_from_file(path);

  gtk_signal_connect(GTK_OBJECT (window), "destroy",
             GTK_SIGNAL_FUNC (destroy), NULL);

  gtk_container_add(GTK_CONTAINER (window), image);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}

