<?php

declare(strict_types=1);

namespace Gtk;

final class Button extends Widget
{
    public function __construct(string $label = 'Button')
    {
        parent::__construct(gtk_button_new($label));
    }
}
