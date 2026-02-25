<?php

declare(strict_types=1);

namespace Gtk;

final class Box extends Widget
{
    public const ORIENTATION_HORIZONTAL = 0;
    public const ORIENTATION_VERTICAL = 1;

    public function __construct(int $orientation = self::ORIENTATION_VERTICAL, int $spacing = 6)
    {
        parent::__construct(gtk_box_new($orientation, $spacing));
    }

    public function append(Widget $child): self
    {
        gtk_box_append($this->id, $child->id());
        return $this;
    }
}
