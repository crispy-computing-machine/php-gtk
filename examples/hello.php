<?php

declare(strict_types=1);

require_once __DIR__ . '/../php-wrapper/Gtk/Widget.php';
require_once __DIR__ . '/../php-wrapper/Gtk/Window.php';
require_once __DIR__ . '/../php-wrapper/Gtk/Box.php';
require_once __DIR__ . '/../php-wrapper/Gtk/Button.php';
require_once __DIR__ . '/../php-wrapper/Gtk/Label.php';
require_once __DIR__ . '/../php-wrapper/Gtk/Entry.php';
require_once __DIR__ . '/../php-wrapper/Gtk/Application.php';

$app = new Gtk\Application('PHP GTK 8.4 Demo', 680, 440);
$app->addLabel('Welcome to PHP GTK!');
$app->addEntry('Type here...');
$app->addButton('Submit');
$app->run();
