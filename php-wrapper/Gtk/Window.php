<?php

declare(strict_types=1);

namespace Gtk;

final class Window extends Widget
{
    public function __construct(string $title = 'PHP GTK Window', int $width = 800, int $height = 600)
    {
        parent::__construct(gtk_window_new($title, $width, $height));
    }

    public function setTitle(string $title): self
    {
        gtk_window_set_title($this->id, $title);
        return $this;
    }

    public function setDefaultSize(int $width, int $height): self
    {
        gtk_window_set_default_size($this->id, $width, $height);
        return $this;
    }

    public function setChild(Widget $child): self
    {
        gtk_window_set_child($this->id, $child->id());
        return $this;
    }

    public function present(): void
    {
        gtk_window_show($this->id);
    }
}
