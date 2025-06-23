#pragma once

struct UIType {
    struct UIType* parent;
};

struct UI {
    struct UIType* type;

    struct UI *parent, *child;
};
