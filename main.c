
#include <stdlib.h> // for system()
#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <ctype.h>
#include <gdk/gdkx.h> // Add this header for X11-specific GDK functions
#include <X11/extensions/XTest.h>
#include <X11/Xlib.h>

#define LINE_WIDTH 1.0

char letters[26] = {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'Z', 'X', 'C', 'V', 'B', 'N', 'M'};

#define ROWS_COUNT 4
#define LETTERS_PER_ROW (26 / ROWS_COUNT)

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

static int selected_box_x = -1;
static int selected_box_y = -1;

static GtkWidget *window;

static gboolean hide_window()
{
    gtk_widget_hide(window);

    return G_SOURCE_REMOVE; // Don't repeat the timeout
}

void click_at_coordinates(int x, int y)
{
    // Hide window and ensure it's hidden
    gtk_widget_hide(window);
    while (gtk_events_pending())
    {
        gtk_main_iteration();
    }
    g_usleep(1000000); // 1 second delay

    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    if (!xdisplay)
    {
        g_print("Could not get X display\n");
        return;
    }

    // Try moving mouse using the display root window directly
    Window root = DefaultRootWindow(xdisplay);
    XWarpPointer(xdisplay, None, root, 0, 0, 0, 0, x, y);
    XSync(xdisplay, False);
    XFlush(xdisplay);
    g_print("Tried moving pointer to %d, %d\n", x, y);

    g_usleep(500000); // 500ms delay after move

    // Click events
    XTestFakeButtonEvent(xdisplay, 1, True, CurrentTime);
    XSync(xdisplay, False);
    XFlush(xdisplay);
    g_print("Sent mouse press\n");

    g_usleep(100000);

    XTestFakeButtonEvent(xdisplay, 1, False, CurrentTime);
    XSync(xdisplay, False);
    XFlush(xdisplay);
    g_print("Sent mouse release\n");

    gtk_widget_show(window);
    gtk_widget_grab_focus(window);
}

static void draw_letters_in_box(cairo_t *cr, int row, int col, double x, double y, int box_width, int box_height, int highlighted_type)
{
    if (highlighted_type == 1)
    {
        selected_box_x = x;
        selected_box_y = y;

        printf("%0.2f", x);
        printf("%0.2f", y);
    }

    char text[3];
    get_letter_combination(text, row, col);

    // Set text color to white with some transparency
    if (highlighted_type != -1)
    {
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.8); // Yellow for highlighted
    }
    else
    {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8); // White for normal
    }

    if (highlighted_type != 1)
    {
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

    double small_box_width = box_width / LETTERS_PER_ROW;
    double small_box_height = box_height / ROWS_COUNT;

    printf("%0.2f ABC %0.2f", small_box_height, small_box_width);

    if (highlighted_type == 1)
    {
        int index = 0;

        for (int i = 0; i < ROWS_COUNT; i++)
        {
            for (int j = 0; j < LETTERS_PER_ROW; j++)
            {
                char text[2];             // Buffer for single character + null terminator
                text[0] = letters[index]; // Replace 'index' with your array index
                text[1] = '\0';

                index++;

                // Use the smaller dimension for font sizing
                cairo_set_font_size(cr, 12);

                cairo_text_extents_t extents;
                cairo_text_extents(cr, text, &extents);

                double text_x = (j * small_box_width) + (x + extents.x_bearing + 6);
                double text_y = (i * small_box_height) + (y - extents.y_bearing + 6);

                cairo_move_to(cr, text_x, text_y);
                cairo_show_text(cr, text);

                cairo_rectangle(cr, x + (j * small_box_width), y + (i * small_box_height), small_box_width, small_box_height);
                cairo_stroke(cr);
            }
        }
    }
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

            // highlight_col == -1 = Row is highlighted
            // highlight_col != -1 = A box is highlighted

            int highlighted_type = (row == highlight_row) ? ((highlight_col == -1) ? 0 : highlight_col == col ? 1
                                                                                                              : -1)
                                                          : -1;

            // highlighted_type == -1 = Nothing is highlighted
            // highlighted_type == 0 = Row is highlighted
            // highlighted_type == 1 = Box is highlighted

            // Draw box background if highlighted
            if (highlighted_type != -1)
            {
                // Different highlight color for row vs single box
                if (highlighted_type == 0)
                {
                    // Whole row highlight
                    cairo_set_source_rgba(cr, 0.4, 0.2, 0.6, 0.3); // Purple tint for row
                }
                else
                {
                    // Single box highlight
                    cairo_set_source_rgba(cr, 0.4, 0.2, 0.6, 0.3);
                }
                cairo_rectangle(cr, x + 2, y + 1, dim.box_width, dim.box_height);
                cairo_fill(cr);
            }

            // Draw box border
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.5);
            cairo_rectangle(cr, x, y, dim.box_width, dim.box_height);
            cairo_stroke(cr);

            // Draw letters
            draw_letters_in_box(cr, row, col, x, y, dim.box_width, dim.box_height, highlighted_type);
        }
    }

    return FALSE;
}

int find_letter_index(char letter)
{
    for (int i = 0; i < 26; i++)
    {
        if (letters[i] == letter)
        {
            return i;
        }
    }
    return -1; // Return -1 if letter not found
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event->keyval == GDK_KEY_space) {
        GdkWindow *gdk_window = gtk_widget_get_window(widget);
        if (gdk_window != NULL) {
            static gboolean click_through = FALSE;
            click_through = !click_through;
            
            if (click_through) {
                // Use gtk_widget_set_opacity for click-through
                gtk_widget_set_opacity(widget, 0.0);
                // Make window ignore input events
                gdk_window_set_events(gdk_window, 0);  // No events
            } else {
                // Restore opacity
                gtk_widget_set_opacity(widget, 1.0);
                // Restore normal event handling
                gdk_window_set_events(gdk_window, GDK_ALL_EVENTS_MASK);
            }
        }
        return TRUE;
    }



    if (event->keyval == GDK_KEY_Escape)
    {
        gtk_main_quit();
        return TRUE;
    }

    if (event->keyval == GDK_KEY_Return)
    {
        key_state.waiting_for_second = FALSE;

        highlight_col = -1;
        highlight_row = -1;

        gtk_widget_queue_draw(widget);
        return TRUE;
    }

    // Convert key to uppercase character
    char key = toupper(event->keyval);

    if (highlight_col != -1)
    {
        int index = find_letter_index(key);
        const int rowIndex = (index / LETTERS_PER_ROW); // 12/6 = 2
        const int colIndex = (index % LETTERS_PER_ROW); // 12%6 = 0

        cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));

        cairo_set_source_rgb(cr, 1, 0, 0); // Red color

        // Move to position
        cairo_rectangle(cr, selected_box_x + (colIndex * 26) + 13, selected_box_y + (rowIndex * 26) + 13, 0.2, 0.2);
        cairo_stroke(cr);

        click_at_coordinates(selected_box_x + (colIndex * 26) + 13, selected_box_y + (rowIndex * 26) + 13);

        highlight_row = -1;
        highlight_col = -1;
        key_state.waiting_for_second = FALSE;

        printf("Row: %d, Col: %d\n", rowIndex, colIndex);
    }
    else
    {
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

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

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