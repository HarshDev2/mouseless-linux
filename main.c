#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#define LINE_WIDTH 1.0

typedef struct
{
    int rows;
    int cols;
    int box_width;
    int box_height;
    int box_size;
} GridDimensions;

// Structure to track key input state
typedef struct
{
    char first_key;
    gboolean waiting_for_second;
} KeyState;

static KeyState key_state = {0, FALSE};

static void get_letter_combination(char *result, int row, int col)
{
    result[0] = 'A' + row;
    result[1] = 'A' + col;
    result[2] = '\0';
}

static GridDimensions calculate_grid_dimensions(int screen_width, int screen_height)
{
    GridDimensions dim;
    dim.rows = 16;
    dim.cols = 16;
    dim.box_width = screen_width / dim.cols;
    dim.box_height = screen_height / dim.rows;
    dim.box_size = fmin(screen_width / dim.cols, screen_height / dim.rows);
    return dim;
}

// Global variables to track the currently highlighted box
static int highlight_row = -1;
static int highlight_col = -1;

static void draw_letters_in_box(cairo_t *cr, int row, int col, double x, double y, int box_width, int box_height, gboolean highlighted)
{
    char text[3];
    get_letter_combination(text, row, col);

    // Set text color to white with some transparency
    if (highlighted)
    {
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.8); // Yellow for highlighted
    }
    else
    {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8); // White for normal
    }

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

    // Use the smaller dimension for font sizing
    double min_size = fmin(box_width, box_height);
    double font_size = min_size * 0.4;
    cairo_set_font_size(cr, font_size);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, text, &extents);

    double text_x = x + (box_width - extents.width) / 2 - extents.x_bearing;
    double text_y = y + (box_height + extents.height) / 2;

    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, text);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    int width = allocation.width;
    int height = allocation.height;

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    GridDimensions dim = calculate_grid_dimensions(width, height);

    int start_x = 0;
    int start_y = 0;

    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.5);
    cairo_set_line_width(cr, LINE_WIDTH);

    for (int row = 0; row < dim.rows; row++)
    {
        for (int col = 0; col < dim.cols; col++)
        {
            int x = start_x + col * dim.box_width;
            int y = start_y + row * dim.box_height;

            // Check if this box should be highlighted
            gboolean is_highlighted = (row == highlight_row &&
                                       (highlight_col == -1 || col == highlight_col));

            // Draw box background if highlighted
            if (is_highlighted)
            {
                // Different highlight color for row vs single box
                if (highlight_col == -1)
                {
                    // Whole row highlight
                    cairo_set_source_rgba(cr, 0.4, 0.2, 0.6, 0.3); // Purple tint for row
                }
                else
                {
                    // Single box highlight
                    cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.5);
                }
                cairo_rectangle(cr, x + 2, y + 1, dim.box_width, dim.box_height);
                cairo_fill(cr);
            }

            // Draw box border
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.5);
            cairo_rectangle(cr, x + 2, y + 1, dim.box_width, dim.box_height);
            cairo_stroke(cr);

            // Draw letters
            draw_letters_in_box(cr, row, col, x, y, dim.box_width, dim.box_height, is_highlighted);
        }
    }

    return FALSE;
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        gtk_main_quit();
        return TRUE;
    }

    if (event->keyval == GDK_KEY_Return)
    {
        // gtk_main_quit();
        return TRUE;
    }

    // Convert key to uppercase character
    char key = toupper(event->keyval);

    // Check if it's a letter A-P (since we have 16x16 grid)
    if (key >= 'A' && key <= 'P')
    {
        if (!key_state.waiting_for_second)
        {
            // First key press
            key_state.first_key = key;
            key_state.waiting_for_second = TRUE;
            highlight_row = key - 'A';
            highlight_col = -1; // Clear previous column highlight
        }
        else
        {
            // Second key press
            highlight_col = key - 'A';
            key_state.waiting_for_second = FALSE;
        }

        // Redraw the widget
        gtk_widget_queue_draw(widget);
        return TRUE;
    }

    return FALSE;
}

static void on_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer data)
{
    gtk_widget_queue_draw(widget);
}

static void on_window_destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

static void on_window_realize(GtkWidget *widget, gpointer data)
{
    GdkWindow *gdk_window = gtk_widget_get_window(widget);
    if (gdk_window != NULL)
    {
        cairo_region_t *empty_region = cairo_region_create();
        gdk_window_input_shape_combine_region(gdk_window, empty_region, 0, 0);
        cairo_region_destroy(empty_region);
    }
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Letter Combinations Grid");

    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_NOTIFICATION);
    gtk_window_set_accept_focus(GTK_WINDOW(window), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual != NULL && gdk_screen_is_composited(screen))
    {
        gtk_widget_set_visual(window, visual);
    }

    gtk_widget_set_app_paintable(window, TRUE);
    gtk_window_fullscreen(GTK_WINDOW(window));
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), drawing_area);

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy), NULL);
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(on_window_realize), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "size-allocate", G_CALLBACK(on_resize), NULL);

    gtk_widget_show_all(window);

    GdkWindow *gdk_window = gtk_widget_get_window(window);
    if (gdk_window != NULL)
    {
        // Set input focus to our window
        gtk_window_present(GTK_WINDOW(window));
        gtk_widget_grab_focus(window);

        // Grab keyboard input
        GdkDisplay *display = gdk_display_get_default();
        GdkSeat *seat = gdk_display_get_default_seat(display);
        gdk_seat_grab(seat,
                      gdk_window,
                      GDK_SEAT_CAPABILITY_KEYBOARD,
                      TRUE, // owner_events
                      NULL, // cursor
                      NULL, // event
                      NULL, // prepare_func
                      NULL  // prepare_func_data
        );
    }

    gtk_main();

    return 0;
}