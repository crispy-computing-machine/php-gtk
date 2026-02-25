<?php

declare(strict_types=1);

namespace Gtk;

final class Application
{
    private Window $window;
    private Box $root;

    public function __construct(string $title = 'PHP GTK Application', int $width = 800, int $height = 600)
    {
        $this->window = new Window($title, $width, $height);
        $this->root = new Box(Box::ORIENTATION_VERTICAL, 8);
        $this->window->setChild($this->root);
    }

    public function addButton(string $label): Button
    {
        $button = new Button($label);
        $this->root->append($button);
        return $button;
    }

    public function addLabel(string $text): Label
    {
        $label = new Label($text);
        $this->root->append($label);
        return $label;
    }

    public function addEntry(string $default = ''): Entry
    {
        $entry = new Entry($default);
        $this->root->append($entry);
        return $entry;
    }

    public function window(): Window
    {
        return $this->window;
    }

    public function run(): void
    {
        $this->window->present();
        gtk_main();
    }
}
