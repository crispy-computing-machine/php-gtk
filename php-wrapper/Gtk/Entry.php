<?php

declare(strict_types=1);

namespace Gtk;

final class Entry extends Widget
{
    public function __construct(string $text = '')
    {
        parent::__construct(gtk_entry_new($text));
    }

    public function setText(string $text): self
    {
        gtk_entry_set_text($this->id, $text);
        return $this;
    }

    public function text(): string
    {
        return gtk_entry_get_text($this->id);
    }
}
