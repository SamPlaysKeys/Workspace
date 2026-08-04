---
layout: page
title: "Workspace Docs"
permalink: /
---

Welcome to the personal documentation workspace. Below is the documentation organized by category.

{% assign sorted_pages = site.pages | sort: "title" %}
{% assign grouped_categories = "Homelab,Guides,GitOps,Learning Path,Platform,Talks,Troubleshooting" | split: "," %}

{% for cat in grouped_categories %}
  {% assign has_items = false %}
  {% for p in sorted_pages %}
    {% if p.category == cat %}
      {% assign has_items = true %}
    {% endif %}
  {% endfor %}

  {% if has_items %}
    <div class="category-section">
      <h2 id="{{ cat | slugify }}">{{ cat }}</h2>
      <hr>

      <!-- Top-level documents (no subcategory) -->
      {% assign top_level_found = false %}
      {% for p in sorted_pages %}
        {% if p.category == cat %}
          {% unless p.subcategory %}
            {% assign top_level_found = true %}
          {% endunless %}
        {% endif %}
      {% endfor %}

      {% if top_level_found %}
        <ul class="docs-list">
        {% for p in sorted_pages %}
          {% if p.category == cat %}
            {% unless p.subcategory %}
              <li>
                <a href="{{ p.url | relative_url }}"><strong>{{ p.title | default: p.name }}</strong></a>
                {% if p.type %} <span class="badge badge-type">{{ p.type }}</span>{% endif %}
                {% if p.status %} <span class="badge badge-status">{{ p.status }}</span>{% endif %}
                {% if p.tags %}
                  {% for tag in p.tags %}
                    <span class="badge badge-tag">{{ tag }}</span>
                  {% endfor %}
                {% endif %}
              </li>
            {% endunless %}
          {% endif %}
        {% endfor %}
        </ul>
      {% endif %}

      <!-- Subcategorized documents -->
      {% assign subcats = "" %}
      {% for p in sorted_pages %}
        {% if p.category == cat %}
          {% if p.subcategory %}
            {% assign subcats = subcats | append: p.subcategory | append: "|" %}
          {% endif %}
        {% endif %}
      {% endfor %}

      {% assign subcats_arr = subcats | split: "|" | uniq | sort %}
      {% for subcat in subcats_arr %}
        {% if subcat != "" %}
          <div class="subcategory-section">
            <h3 class="subcategory-title">{{ subcat }}</h3>
            <ul class="docs-list">
            {% for p in sorted_pages %}
              {% if p.category == cat %}
                {% if p.subcategory == subcat %}
                  <li>
                    <a href="{{ p.url | relative_url }}"><strong>{{ p.title | default: p.name }}</strong></a>
                    {% if p.type %} <span class="badge badge-type">{{ p.type }}</span>{% endif %}
                    {% if p.status %} <span class="badge badge-status">{{ p.status }}</span>{% endif %}
                    {% if p.tags %}
                      {% for tag in p.tags %}
                        <span class="badge badge-tag">{{ tag }}</span>
                      {% endfor %}
                    {% endif %}
                  </li>
                {% endif %}
              {% endif %}
            {% endfor %}
            </ul>
          </div>
        {% endif %}
      {% endfor %}
    </div>
  {% endif %}
{% endfor %}

<!-- Dynamic sections for custom categories -->
{% assign all_categories = site.pages | map: "category" | uniq %}
{% for cat in all_categories %}
  {% if cat and cat != "" and cat != "/" %}
    {% unless grouped_categories contains cat %}
      <div class="category-section">
        <h2 id="{{ cat | slugify }}">{{ cat }}</h2>
        <hr>

        <!-- Top-level documents (no subcategory) -->
        {% assign top_level_found = false %}
        {% for p in sorted_pages %}
          {% if p.category == cat %}
            {% unless p.subcategory %}
              {% assign top_level_found = true %}
            {% endunless %}
          {% endif %}
        {% endfor %}

        {% if top_level_found %}
          <ul class="docs-list">
          {% for p in sorted_pages %}
            {% if p.category == cat %}
              {% unless p.subcategory %}
                <li>
                  <a href="{{ p.url | relative_url }}"><strong>{{ p.title | default: p.name }}</strong></a>
                  {% if p.type %} <span class="badge badge-type">{{ p.type }}</span>{% endif %}
                  {% if p.status %} <span class="badge badge-status">{{ p.status }}</span>{% endif %}
                  {% if p.tags %}
                    {% for tag in p.tags %}
                      <span class="badge badge-tag">{{ tag }}</span>
                    {% endfor %}
                  {% endif %}
                </li>
              {% endunless %}
            {% endif %}
          {% endfor %}
          </ul>
        {% endif %}

        <!-- Subcategorized documents -->
        {% assign subcats = "" %}
        {% for p in sorted_pages %}
          {% if p.category == cat %}
            {% if p.subcategory %}
              {% assign subcats = subcats | append: p.subcategory | append: "|" %}
            {% endif %}
          {% endif %}
        {% endfor %}

        {% assign subcats_arr = subcats | split: "|" | uniq | sort %}
        {% for subcat in subcats_arr %}
          {% if subcat != "" %}
            <div class="subcategory-section">
              <h3 class="subcategory-title">{{ subcat }}</h3>
              <ul class="docs-list">
              {% for p in sorted_pages %}
                {% if p.category == cat %}
                  {% if p.subcategory == subcat %}
                    <li>
                      <a href="{{ p.url | relative_url }}"><strong>{{ p.title | default: p.name }}</strong></a>
                      {% if p.type %} <span class="badge badge-type">{{ p.type }}</span>{% endif %}
                      {% if p.status %} <span class="badge badge-status">{{ p.status }}</span>{% endif %}
                      {% if p.tags %}
                        {% for tag in p.tags %}
                          <span class="badge badge-tag">{{ tag }}</span>
                        {% endfor %}
                      {% endif %}
                    </li>
                  {% endif %}
                {% endif %}
              {% endfor %}
              </ul>
            </div>
          {% endif %}
        {% endfor %}
      </div>
    {% endunless %}
  {% endif %}
{% endfor %}

<style>
  .category-section { margin-top: 2.5rem; margin-bottom: 2.5rem; }
  .category-section h2 { margin-bottom: 0.5rem; font-size: 1.8rem; color: #111; }
  .category-section hr { border: 0; border-top: 1px solid #eee; margin-top: 0; margin-bottom: 1rem; }
  .docs-list { list-style-type: none; padding-left: 0; }
  .docs-list li { padding: 0.4rem 0; font-size: 1.1rem; border-bottom: 1px dashed #fafafa; }
  .badge {
    padding: 0.1rem 0.4rem; border-radius: 4px; font-size: 0.75rem; margin-left: 0.5rem; border: 1px solid; display: inline-block;
  }
  .badge-type { background-color: #f4f6fa; color: #5c6bc0; border-color: #d2d7f3; }
  .badge-status { background-color: #f1f8e9; color: #4caf50; border-color: #c5e1a5; }
  .badge-tag { background-color: #fafafa; color: #757575; border-color: #e0e0e0; }
  .subcategory-section { margin-left: 1.5rem; border-left: 2px solid #eee; padding-left: 1rem; margin-top: 1rem; margin-bottom: 1rem; }
  .subcategory-title { font-size: 1.2rem; color: #555; margin-bottom: 0.5rem; margin-top: 1rem; }
</style>
