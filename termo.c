#include <gtk/gtk.h>

#define TERMO_NAME "termo"
#define TERMO_VERSION "0.1"
#define TERMO_DEFAULT_SHELL "/bin/sh"

#define TERMO_FONT_FAMILY "JetBrains Mono"
#define TERMO_FONT_SIZE 14

static GString *command_buffer;

static void execute_command_and_update_text_view(GtkWidget *text_view,
                                                 const gchar *cmd) {

  if (g_strcmp0(cmd, "exit") == 0) {
    GtkWidget *top_level_window =
        gtk_widget_get_ancestor(text_view, GTK_TYPE_WINDOW);
    if (GTK_IS_WINDOW(top_level_window)) {
      gtk_window_close(GTK_WINDOW(top_level_window));
    }
    return;
  }

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  if (g_strcmp0(cmd, "clear") == 0) {
    gtk_text_buffer_set_text(buffer, "", -1);
    return;
  }

  const gchar *shell = g_getenv("SHELL"); // Get the user's default shell
  if (shell == NULL) {
    shell = TERMO_DEFAULT_SHELL; // Fallback to /bin/sh if SHELL is not set
  }
  const gchar *escaped_cmd = g_strescape(cmd, "\"");

  gchar *command_line = g_strdup_printf(
      "%s -c \"%s\"", shell, escaped_cmd); // Construct the command line
  gchar *output;
  GError *error = NULL;

  // Run the command and capture its output
  if (g_spawn_command_line_sync(command_line, &output, NULL, NULL, &error)) {
    // Get the text buffer associated with the text view

    // Set the text buffer's content to the command's output
    gtk_text_buffer_set_text(buffer, output, -1);
  } else {
    g_printerr("Error running command: %s\n", error->message);
    g_clear_error(&error);
  }

  g_free(output);
  g_free(command_line);
}

static gboolean on_key_press(GtkEventControllerKey *controller, guint keyval,
                             guint keycode, GdkModifierType state,
                             gpointer user_data) {
  GtkWidget *text_view = GTK_WIDGET(user_data);
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  GtkTextIter end_iter;

  // Handle Enter key
  if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
    execute_command_and_update_text_view(text_view, command_buffer->str);
    g_string_erase(command_buffer, 0, -1); // Clear the command buffer
  } else if (keyval == GDK_KEY_BackSpace) {
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    if (!gtk_text_iter_is_start(&end_iter)) { // Check if it's not the beginning
      // Use gtk_text_buffer_backspace to delete the character before the
      // end_iter
      gtk_text_buffer_backspace(buffer, &end_iter, TRUE, TRUE);
    }
    if (command_buffer->len > 0) {
      g_string_erase(command_buffer, command_buffer->len - 1, 1);
    }
  } else {
    // Append regular key presses to the buffer, for simplicity we use the
    // keyval as char
    if (g_unichar_isprint(gdk_keyval_to_unicode(keyval))) {
      g_string_append_unichar(command_buffer, gdk_keyval_to_unicode(keyval));
      gchar *utf8_char = g_strdup_printf("%c", gdk_keyval_to_unicode(keyval));
      gtk_text_buffer_get_end_iter(buffer, &end_iter);
      gtk_text_buffer_insert(buffer, &end_iter, utf8_char,
                             -1); // Insert the character at the end
      g_free(utf8_char);
    }
  }

  return GDK_EVENT_PROPAGATE;
}

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window;

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), TERMO_NAME);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  /* TODO: Add fullscreen option */
  /* gtk_window_fullscreen(GTK_WINDOW(window)); */

  // Create a text view with a monospace font
  GtkWidget *text_view = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view),
                             FALSE); // Make it read-only
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_view),
                              TRUE); // Use monospace font

  gtk_window_set_child(GTK_WINDOW(window), text_view);

  // Apply CSS for monospace font
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(
      provider,
      g_strdup_printf("textview {  font-family: %s; font-size: %dpt; "
                      "caret-color: rgb(255,0,80);}",
                      TERMO_FONT_FAMILY, TERMO_FONT_SIZE),
      -1);
  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  // Create a key event controller and attach it to the text view
  GtkEventController *key_controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(text_view, GTK_EVENT_CONTROLLER(key_controller));

  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_press),
                   text_view);

  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  GtkApplication *app;
  int status;

  command_buffer = g_string_new(NULL);

  app = gtk_application_new("org.gtk.termo", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
