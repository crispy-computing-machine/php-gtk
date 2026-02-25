<?php

declare(strict_types=1);

namespace Gtk;

final class Label extends Widget
{
    public function __construct(string $text = '')
    {
        parent::__construct(gtk_label_new($text));
    }
}
