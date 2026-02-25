# php-gtk (PHP 8.4)

A native PHP extension prototype for building GTK UI applications from PHP.

## What is included

### Extension userland functions

- Window functions:
  - `gtk_window_new(string $title = "PHP GTK Window", int $width = 800, int $height = 600): int`
  - `gtk_window_set_title(int $windowId, string $title): bool`
  - `gtk_window_set_default_size(int $windowId, int $width, int $height): bool`
  - `gtk_window_set_child(int $windowId, int $childId): bool`
  - `gtk_window_show(int $windowId): bool`

- Container functions:
  - `gtk_box_new(int $orientation = 1, int $spacing = 6): int`
  - `gtk_box_append(int $boxId, int $childId): bool`

- Widget functions:
  - `gtk_button_new(string $label = "Button"): int`
  - `gtk_label_new(string $text = ""): int`
  - `gtk_entry_new(string $text = ""): int`
  - `gtk_entry_set_text(int $entryId, string $text): bool`
  - `gtk_entry_get_text(int $entryId): string`
  - `gtk_widget_show(int $widgetId): bool`
  - `gtk_widget_show_all(int $widgetId): bool`

- Event loop functions:
  - `gtk_main(): void`
  - `gtk_main_quit(): void`

- Compatibility aliases:
  - `gtk_create_window(...)`
  - `gtk_create_control(...)`
  - `gtk_show_window(...)`

### PHP wrapper classes

- `Gtk\Widget`
- `Gtk\Window`
- `Gtk\Box`
- `Gtk\Button`
- `Gtk\Label`
- `Gtk\Entry`
- `Gtk\Application`

## Build on Linux/macOS (typical)

Requirements:

- PHP 8.4 development headers (`phpize`, `php-config`)
- GTK3 development packages (`gtk+-3.0`, pkg-config)

```bash
phpize
./configure --enable-gtk
make
```

Then load the compiled module:

```ini
extension=gtk
```

## Windows/AppVeyor

- Added `appveyor.yml` and `scripts/appveyor-build.ps1`.
- Script installs PHP (if needed) and validates wrapper PHP files.
- Native Windows extension compilation now runs only when `GTK_SDK_ROOT` points to an MSVC-compatible GTK SDK containing:
  - `include\gtk-3.0`
  - `lib`
- Without `GTK_SDK_ROOT`, CI intentionally skips native compile entirely (including configure.js/buildconf/nmake) and still runs lint + artifact packaging.

## Example

```php
<?php
require_once __DIR__ . '/php-wrapper/Gtk/Widget.php';
require_once __DIR__ . '/php-wrapper/Gtk/Window.php';
require_once __DIR__ . '/php-wrapper/Gtk/Box.php';
require_once __DIR__ . '/php-wrapper/Gtk/Button.php';
require_once __DIR__ . '/php-wrapper/Gtk/Label.php';
require_once __DIR__ . '/php-wrapper/Gtk/Entry.php';
require_once __DIR__ . '/php-wrapper/Gtk/Application.php';

$app = new Gtk\Application('PHP GTK Demo', 640, 420);
$app->addLabel('Hello from PHP + GTK');
$app->addEntry('Type here');
$app->addButton('Click me');
$app->run();
```

Or run the sample:

```bash
php examples/hello.php
```
