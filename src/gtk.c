#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_gtk.h"

#include <gtk/gtk.h>
#include <strings.h>
#include "Zend/zend_exceptions.h"

ZEND_BEGIN_MODULE_GLOBALS(gtk)
    gboolean initialized;
    zend_long next_widget_id;
    HashTable widgets;
    HashTable windows;
ZEND_END_MODULE_GLOBALS(gtk)

ZEND_DECLARE_MODULE_GLOBALS(gtk)

#define GTK_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(gtk, v)

static void php_gtk_init_globals(zend_gtk_globals *gtk_globals)
{
    gtk_globals->initialized = FALSE;
    gtk_globals->next_widget_id = 1;
}

static void php_gtk_window_dtor(zval *zv)
{
    GtkWidget *widget = (GtkWidget *) Z_PTR_P(zv);

    if (widget != NULL) {
        gtk_widget_destroy(widget);
    }
}

static void php_gtk_ensure_initialized(void)
{
    if (!GTK_G(initialized)) {
        int argc = 0;
        char **argv = NULL;

        gtk_init(&argc, &argv);
        GTK_G(initialized) = TRUE;
    }
}

static zend_long php_gtk_register_widget(GtkWidget *widget, bool is_window)
{
    zval holder;
    zend_long widget_id = GTK_G(next_widget_id)++;

    ZVAL_PTR(&holder, widget);
    zend_hash_index_update(&GTK_G(widgets), widget_id, &holder);

    if (is_window) {
        zend_hash_index_update(&GTK_G(windows), widget_id, &holder);
    }

    return widget_id;
}

static GtkWidget *php_gtk_fetch_widget(zend_long widget_id)
{
    zval *widget_zv = zend_hash_index_find(&GTK_G(widgets), widget_id);

    if (widget_zv == NULL) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Unknown widget id %ld", widget_id);
        return NULL;
    }

    return (GtkWidget *) Z_PTR_P(widget_zv);
}

PHP_FUNCTION(gtk_window_new)
{
    char *title = "PHP GTK Window";
    size_t title_len = sizeof("PHP GTK Window") - 1;
    zend_long width = 800;
    zend_long height = 600;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|sll", &title, &title_len, &width, &height) == FAILURE) {
        RETURN_THROWS();
    }

    php_gtk_ensure_initialized();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), (gint) width, (gint) height);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    RETURN_LONG(php_gtk_register_widget(window, true));
}

PHP_FUNCTION(gtk_window_set_title)
{
    zend_long window_id;
    char *title;
    size_t title_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ls", &window_id, &title, &title_len) == FAILURE) {
        RETURN_THROWS();
    }

    GtkWidget *window = php_gtk_fetch_widget(window_id);
    if (window == NULL) {
        RETURN_THROWS();
    }

    if (!GTK_IS_WINDOW(window)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Widget id %ld is not a GtkWindow", window_id);
        RETURN_THROWS();
    }

    gtk_window_set_title(GTK_WINDOW(window), title);
    RETURN_TRUE;
}

PHP_FUNCTION(gtk_window_set_default_size)
{
    zend_long window_id;
    zend_long width;
    zend_long height;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lll", &window_id, &width, &height) == FAILURE) {
        RETURN_THROWS();
    }

    GtkWidget *window = php_gtk_fetch_widget(window_id);
    if (window == NULL) {
        RETURN_THROWS();
    }

    if (!GTK_IS_WINDOW(window)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Widget id %ld is not a GtkWindow", window_id);
        RETURN_THROWS();
    }

    gtk_window_set_default_size(GTK_WINDOW(window), (gint) width, (gint) height);
    RETURN_TRUE;
}

PHP_FUNCTION(gtk_window_set_child)
{
    zend_long window_id;
    zend_long child_id;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &window_id, &child_id) == FAILURE) {
        RETURN_THROWS();
    }

    GtkWidget *window = php_gtk_fetch_widget(window_id);
    GtkWidget *child = php_gtk_fetch_widget(child_id);

    if (window == NULL || child == NULL) {
        RETURN_THROWS();
    }

    if (!GTK_IS_WINDOW(window)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Widget id %ld is not a GtkWindow", window_id);
        RETURN_THROWS();
    }

    gtk_container_add(GTK_CONTAINER(window), child);
    RETURN_TRUE;
}

PHP_FUNCTION(gtk_window_show)
{
    zend_long window_id;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &window_id) == FAILURE) {
        RETURN_THROWS();
    }

    GtkWidget *window = php_gtk_fetch_widget(window_id);
    if (window == NULL) {
        RETURN_THROWS();
    }

    gtk_widget_show_all(window);
    RETURN_TRUE;
}

PHP_FUNCTION(gtk_box_new)
{
    zend_long orientation = 1;
    zend_long spacing = 6;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|ll", &orientation, &spacing) == FAILURE) {
        RETURN_THROWS();
    }

    php_gtk_ensure_initialized();

    GtkOrientation gtk_orientation = orientation == 0 ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
    GtkWidget *box = gtk_box_new(gtk_orientation, (gint) spacing);

    RETURN_LONG(php_gtk_register_widget(box, false));
}

PHP_FUNCTION(gtk_box_append)
{
    zend_long box_id;
    zend_long child_id;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &box_id, &child_id) == FAILURE) {
        RETURN_THROWS();
    }

    GtkWidget *box = php_gtk_fetch_widget(box_id);
    GtkWidget *child = php_gtk_fetch_widget(child_id);

    if (box == NULL || child == NULL) {
        RETURN_THROWS();
    }

    if (!GTK_IS_BOX(box)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Widget id %ld is not a GtkBox", box_id);
        RETURN_THROWS();
    }

    gtk_box_pack_start(GTK_BOX(box), child, FALSE, FALSE, 0);
    RETURN_TRUE;
}

PHP_FUNCTION(gtk_button_new)
{
    char *label = "Button";
    size_t label_len = sizeof("Button") - 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s", &label, &label_len) == FAILURE) {
        RETURN_THROWS();
    }

    php_gtk_ensure_initialized();

    GtkWidget *button = gtk_button_new_with_label(label);
    RETURN_LONG(php_gtk_register_widget(button, false));
}

PHP_FUNCTION(gtk_label_new)
{
    char *text = "";
    size_t text_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s", &text, &text_len) == FAILURE) {
        RETURN_THROWS();
    }

    php_gtk_ensure_initialized();

    GtkWidget *label = gtk_label_new(text);
    RETURN_LONG(php_gtk_register_widget(label, false));
}

PHP_FUNCTION(gtk_entry_new)
{
    char *text = "";
    size_t text_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s", &text, &text_len) == FAILURE) {
        RETURN_THROWS();
    }

    php_gtk_ensure_initialized();

    GtkWidget *entry = gtk_entry_new();
    if (text_len > 0) {
        gtk_entry_set_text(GTK_ENTRY(entry), text);
    }

    RETURN_LONG(php_gtk_register_widget(entry, false));
}

PHP_FUNCTION(gtk_entry_set_text)
{
    zend_long entry_id;
    char *text;
    size_t text_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ls", &entry_id, &text, &text_len) == FAILURE) {
        RETURN_THROWS();
    }

    GtkWidget *entry = php_gtk_fetch_widget(entry_id);
    if (entry == NULL) {
        RETURN_THROWS();
    }

    if (!GTK_IS_ENTRY(entry)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Widget id %ld is not a GtkEntry", entry_id);
        RETURN_THROWS();
    }

    gtk_entry_set_text(GTK_ENTRY(entry), text);
    RETURN_TRUE;
}

PHP_FUNCTION(gtk_entry_get_text)
{
    zend_long entry_id;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &entry_id) == FAILURE) {
        RETURN_THROWS();
    }

    GtkWidget *entry = php_gtk_fetch_widget(entry_id);
    if (entry == NULL) {
        RETURN_THROWS();
    }

    if (!GTK_IS_ENTRY(entry)) {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Widget id %ld is not a GtkEntry", entry_id);
        RETURN_THROWS();
    }

    RETURN_STRING(gtk_entry_get_text(GTK_ENTRY(entry)));
}

PHP_FUNCTION(gtk_widget_show)
{
    zend_long widget_id;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &widget_id) == FAILURE) {
        RETURN_THROWS();
    }

    GtkWidget *widget = php_gtk_fetch_widget(widget_id);
    if (widget == NULL) {
        RETURN_THROWS();
    }

    gtk_widget_show(widget);
    RETURN_TRUE;
}

PHP_FUNCTION(gtk_widget_show_all)
{
    zend_long widget_id;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &widget_id) == FAILURE) {
        RETURN_THROWS();
    }

    GtkWidget *widget = php_gtk_fetch_widget(widget_id);
    if (widget == NULL) {
        RETURN_THROWS();
    }

    gtk_widget_show_all(widget);
    RETURN_TRUE;
}

PHP_FUNCTION(gtk_main)
{
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_THROWS();
    }

    gtk_main();
}

PHP_FUNCTION(gtk_main_quit)
{
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_THROWS();
    }

    gtk_main_quit();
}

/* Backwards-compatible API */
PHP_FUNCTION(gtk_create_window)
{
    char *title = "PHP GTK Window";
    size_t title_len = sizeof("PHP GTK Window") - 1;
    zend_long width = 800;
    zend_long height = 600;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|sll", &title, &title_len, &width, &height) == FAILURE) {
        RETURN_THROWS();
    }

    php_gtk_ensure_initialized();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), (gint) width, (gint) height);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    RETURN_LONG(php_gtk_register_widget(window, true));
}

PHP_FUNCTION(gtk_create_control)
{
    zend_long parent_id;
    char *type;
    size_t type_len;
    char *label = "";
    size_t label_len = 0;
    zend_long new_widget_id;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ls|s", &parent_id, &type, &type_len, &label, &label_len) == FAILURE) {
        RETURN_THROWS();
    }

    php_gtk_ensure_initialized();

    if (strcasecmp(type, "button") == 0) {
        GtkWidget *button = gtk_button_new_with_label(label_len > 0 ? label : "Button");
        new_widget_id = php_gtk_register_widget(button, false);
    } else if (strcasecmp(type, "label") == 0) {
        GtkWidget *new_label = gtk_label_new(label);
        new_widget_id = php_gtk_register_widget(new_label, false);
    } else if (strcasecmp(type, "entry") == 0) {
        GtkWidget *entry = gtk_entry_new();
        if (label_len > 0) {
            gtk_entry_set_text(GTK_ENTRY(entry), label);
        }
        new_widget_id = php_gtk_register_widget(entry, false);
    } else if (strcasecmp(type, "box") == 0) {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        new_widget_id = php_gtk_register_widget(box, false);
    } else {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Unsupported control type '%s'", type);
        RETURN_THROWS();
    }

    GtkWidget *parent = php_gtk_fetch_widget(parent_id);
    GtkWidget *created = php_gtk_fetch_widget(new_widget_id);

    if (parent == NULL || created == NULL) {
        RETURN_THROWS();
    }

    if (GTK_IS_BOX(parent)) {
        gtk_box_pack_start(GTK_BOX(parent), created, FALSE, FALSE, 0);
    } else if (GTK_IS_WINDOW(parent)) {
        GtkWidget *existing = gtk_bin_get_child(GTK_BIN(parent));
        if (existing == NULL) {
            gtk_container_add(GTK_CONTAINER(parent), created);
        } else if (GTK_IS_BOX(existing)) {
            gtk_box_pack_start(GTK_BOX(existing), created, FALSE, FALSE, 0);
        } else {
            zend_throw_exception_ex(zend_ce_value_error, 0, "Window %ld already has a non-box child", parent_id);
            RETURN_THROWS();
        }
    } else {
        zend_throw_exception_ex(zend_ce_value_error, 0, "Unsupported parent widget id %ld", parent_id);
        RETURN_THROWS();
    }

    RETURN_LONG(new_widget_id);
}

PHP_FUNCTION(gtk_show_window)
{
    ZEND_MN(gtk_window_show)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_window_new, 0, 0, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, title, IS_STRING, 0, "\"PHP GTK Window\"")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, width, IS_LONG, 0, "800")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, height, IS_LONG, 0, "600")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_window_set_title, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, windowId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, title, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_window_set_default_size, 0, 3, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, windowId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_window_set_child, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, windowId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, childId, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_window_show, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, windowId, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_box_new, 0, 0, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, orientation, IS_LONG, 0, "1")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, spacing, IS_LONG, 0, "6")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_box_append, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, boxId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, childId, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_button_new, 0, 0, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, label, IS_STRING, 0, "\"Button\"")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_label_new, 0, 0, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, text, IS_STRING, 0, "\"\"")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_entry_new, 0, 0, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, text, IS_STRING, 0, "\"\"")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_entry_set_text, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, entryId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, text, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_entry_get_text, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, entryId, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_widget_show, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, widgetId, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_widget_show_all, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, widgetId, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_main, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_main_quit, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_create_window, 0, 0, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, title, IS_STRING, 0, "\"PHP GTK Window\"")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, width, IS_LONG, 0, "800")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, height, IS_LONG, 0, "600")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_gtk_create_control, 0, 2, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, parentId, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, type, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, label, IS_STRING, 0, "\"\"")
ZEND_END_ARG_INFO()

static const zend_function_entry gtk_functions[] = {
    PHP_FE(gtk_window_new, arginfo_gtk_window_new)
    PHP_FE(gtk_window_set_title, arginfo_gtk_window_set_title)
    PHP_FE(gtk_window_set_default_size, arginfo_gtk_window_set_default_size)
    PHP_FE(gtk_window_set_child, arginfo_gtk_window_set_child)
    PHP_FE(gtk_window_show, arginfo_gtk_window_show)

    PHP_FE(gtk_box_new, arginfo_gtk_box_new)
    PHP_FE(gtk_box_append, arginfo_gtk_box_append)

    PHP_FE(gtk_button_new, arginfo_gtk_button_new)
    PHP_FE(gtk_label_new, arginfo_gtk_label_new)
    PHP_FE(gtk_entry_new, arginfo_gtk_entry_new)
    PHP_FE(gtk_entry_set_text, arginfo_gtk_entry_set_text)
    PHP_FE(gtk_entry_get_text, arginfo_gtk_entry_get_text)

    PHP_FE(gtk_widget_show, arginfo_gtk_widget_show)
    PHP_FE(gtk_widget_show_all, arginfo_gtk_widget_show_all)

    PHP_FE(gtk_main, arginfo_gtk_main)
    PHP_FE(gtk_main_quit, arginfo_gtk_main_quit)

    /* compatibility aliases */
    PHP_FE(gtk_create_window, arginfo_gtk_create_window)
    PHP_FE(gtk_create_control, arginfo_gtk_create_control)
    PHP_FE(gtk_show_window, arginfo_gtk_window_show)

    PHP_FE_END
};

PHP_MINIT_FUNCTION(gtk)
{
    zend_hash_init(&GTK_G(widgets), 64, NULL, NULL, 1);
    zend_hash_init(&GTK_G(windows), 16, NULL, php_gtk_window_dtor, 1);
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(gtk)
{
    zend_hash_destroy(&GTK_G(widgets));
    zend_hash_destroy(&GTK_G(windows));
    return SUCCESS;
}

PHP_MINFO_FUNCTION(gtk)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "gtk support", "enabled");
    php_info_print_table_row(2, "version", PHP_GTK_VERSION);
    php_info_print_table_row(2, "exported widget constructors", "GtkWindow, GtkBox, GtkButton, GtkLabel, GtkEntry");
    php_info_print_table_end();
}

zend_module_entry gtk_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_GTK_EXTNAME,
    gtk_functions,
    PHP_MINIT(gtk),
    PHP_MSHUTDOWN(gtk),
    NULL,
    NULL,
    PHP_MINFO(gtk),
    PHP_GTK_VERSION,
    PHP_MODULE_GLOBALS(gtk),
    php_gtk_init_globals,
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_GTK
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(gtk)
#endif
