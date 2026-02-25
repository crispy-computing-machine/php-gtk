<?php

declare(strict_types=1);

namespace Gtk;

abstract class Widget
{
    public function __construct(protected int $id)
    {
    }

    public function id(): int
    {
        return $this->id;
    }

    public function show(): void
    {
        gtk_widget_show($this->id);
    }

    public function showAll(): void
    {
        gtk_widget_show_all($this->id);
    }
}
