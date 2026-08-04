---
layout: default
title: "Developer Portal"
permalink: /
---

<div class="homepage-container">
  <!-- Hero Welcome Banner -->
  <div class="hero-section">
    <h1>⚡ Workspace Developer Portal</h1>
    <p>Centralized documentation dashboard for homelab automation, system architecture plans, technical presentations, and practical engineering guides.</p>
  </div>

  {% assign sorted_pages = site.pages | sort: "title" %}
  {% assign active_categories = "Homelab,Guides,Architecture,Talks" | split: "," %}

  <!-- Categories Grid -->
  <div class="category-grid">
    {% for cat in active_categories %}
      
      <!-- Check if this category has active items -->
      {% assign has_items = false %}
      {% assign doc_count = 0 %}
      {% for p in sorted_pages %}
        {% if p.category == cat %}
          {% assign has_items = true %}
          {% assign doc_count = doc_count | plus: 1 %}
        {% endif %}
      {% endfor %}

      {% if has_items %}
        {% if cat == "Homelab" %}{% assign cat_icon = "🏠" %}
        {% elsif cat == "Guides" %}{% assign cat_icon = "📚" %}
        {% elsif cat == "Architecture" %}{% assign cat_icon = "🏛️" %}
        {% elsif cat == "Talks" %}{% assign cat_icon = "🎤" %}
        {% else %}{% assign cat_icon = "📄" %}
        {% endif %}

        <section class="category-card" id="{{ cat | slugify }}">
          <div class="category-card-header">
            <span class="category-icon">{{ cat_icon }}</span>
            <h2>{{ cat }}</h2>
            <span class="doc-count">{{ doc_count }} docs</span>
          </div>

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
            <ul class="home-docs-list">
              {% for p in sorted_pages %}
                {% if p.category == cat %}
                  {% unless p.subcategory %}
                    <li>
                      <a href="{{ p.url | relative_url }}"><strong>{{ p.title | default: p.name }}</strong></a>
                      <div class="doc-meta-badges">
                        {% if p.type %} <span class="badge badge-type">{{ p.type }}</span>{% endif %}
                        {% if p.status %} <span class="badge badge-status">{{ p.status }}</span>{% endif %}
                        {% if p.tags %}
                          {% for tag in p.tags %}
                            <span class="badge badge-tag">{{ tag }}</span>
                          {% endfor %}
                        {% endif %}
                      </div>
                    </li>
                  {% endunless %}
                {% endif %}
              {% endfor %}
            </ul>
          {% endif %}

          <!-- Subcategorized documents -->
          {% assign subcats = "" %}
          {% for p in sorted_pages %}
            {% if p.category == cat and p.subcategory %}
              {% assign subcats = subcats | append: p.subcategory | append: "|" %}
            {% endif %}
          {% endfor %}

          {% assign subcats_arr = subcats | split: "|" | uniq | sort %}
          {% for subcat in subcats_arr %}
            {% if subcat != "" %}
              <div class="home-subcategory-section">
                <h3 class="home-subcategory-title">{{ subcat }}</h3>
                <ul class="home-docs-list">
                  {% for p in sorted_pages %}
                    {% if p.category == cat and p.subcategory == subcat %}
                      <li>
                        <a href="{{ p.url | relative_url }}"><strong>{{ p.title | default: p.name }}</strong></a>
                        <div class="doc-meta-badges">
                          {% if p.type %} <span class="badge badge-type">{{ p.type }}</span>{% endif %}
                          {% if p.status %} <span class="badge badge-status">{{ p.status }}</span>{% endif %}
                          {% if p.tags %}
                            {% for tag in p.tags %}
                              <span class="badge badge-tag">{{ tag }}</span>
                            {% endfor %}
                          {% endif %}
                        </div>
                      </li>
                    {% endif %}
                  {% endfor %}
                </ul>
              </div>
            {% endif %}
          {% endfor %}
        </section>
      {% endif %}
    {% endfor %}

    <!-- Dynamic section for custom categories if any exist -->
    {% assign all_categories = site.pages | map: "category" | uniq %}
    {% for cat in all_categories %}
      {% if cat and cat != "" and cat != "/" %}
        {% unless active_categories contains cat %}
          
          {% assign doc_count = 0 %}
          {% for p in sorted_pages %}
            {% if p.category == cat %}
              {% assign doc_count = doc_count | plus: 1 %}
            {% endif %}
          {% endfor %}

          <section class="category-card" id="{{ cat | slugify }}">
            <div class="category-card-header">
              <span class="category-icon">📁</span>
              <h2>{{ cat }}</h2>
              <span class="doc-count">{{ doc_count }} docs</span>
            </div>

            <!-- Top-level documents -->
            {% assign top_level_found = false %}
            {% for p in sorted_pages %}
              {% if p.category == cat %}
                {% unless p.subcategory %}
                  {% assign top_level_found = true %}
                {% endunless %}
              {% endif %}
            {% endfor %}

            {% if top_level_found %}
              <ul class="home-docs-list">
                {% for p in sorted_pages %}
                  {% if p.category == cat %}
                    {% unless p.subcategory %}
                      <li>
                        <a href="{{ p.url | relative_url }}"><strong>{{ p.title | default: p.name }}</strong></a>
                        <div class="doc-meta-badges">
                          {% if p.type %} <span class="badge badge-type">{{ p.type }}</span>{% endif %}
                          {% if p.status %} <span class="badge badge-status">{{ p.status }}</span>{% endif %}
                          {% if p.tags %}
                            {% for tag in p.tags %}
                              <span class="badge badge-tag">{{ tag }}</span>
                            {% endfor %}
                          {% endif %}
                        </div>
                      </li>
                    {% endunless %}
                  {% endif %}
                {% endfor %}
              </ul>
            {% endif %}

            <!-- Subcategorized -->
            {% assign subcats = "" %}
            {% for p in sorted_pages %}
              {% if p.category == cat and p.subcategory %}
                {% assign subcats = subcats | append: p.subcategory | append: "|" %}
              {% endif %}
            {% endfor %}

            {% assign subcats_arr = subcats | split: "|" | uniq | sort %}
            {% for subcat in subcats_arr %}
              {% if subcat != "" %}
                <div class="home-subcategory-section">
                  <h3 class="home-subcategory-title">{{ subcat }}</h3>
                  <ul class="home-docs-list">
                    {% for p in sorted_pages %}
                      {% if p.category == cat and p.subcategory == subcat %}
                        <li>
                          <a href="{{ p.url | relative_url }}"><strong>{{ p.title | default: p.name }}</strong></a>
                          <div class="doc-meta-badges">
                            {% if p.type %} <span class="badge badge-type">{{ p.type }}</span>{% endif %}
                            {% if p.status %} <span class="badge badge-status">{{ p.status }}</span>{% endif %}
                            {% if p.tags %}
                              {% for tag in p.tags %}
                                <span class="badge badge-tag">{{ tag }}</span>
                              {% endfor %}
                            {% endif %}
                          </div>
                        </li>
                      {% endif %}
                    {% endfor %}
                  </ul>
                </div>
              {% endif %}
            {% endfor %}
          </section>
        {% endunless %}
      {% endif %}
    {% endfor %}
  </div>
</div>

<style>
  .homepage-container {
    padding: 3rem 2rem;
    max-width: 1400px;
    margin: 0 auto;
  }

  .hero-section {
    text-align: center;
    margin-bottom: 3.5rem;
    padding: 3.5rem 2rem;
    background: linear-gradient(135deg, rgba(30, 41, 59, 0.4) 0%, rgba(15, 23, 42, 0.8) 100%);
    border: 1px solid var(--border-light);
    border-radius: 12px;
  }

  .hero-section h1 {
    font-size: 2.75rem;
    font-weight: 800;
    letter-spacing: -0.03em;
    color: var(--text-main);
    margin-bottom: 0.75rem;
  }

  .hero-section p {
    font-size: 1.15rem;
    color: var(--text-muted);
    max-width: 800px;
    margin: 0 auto;
    font-weight: 400;
  }

  /* Grid Layout for Cards */
  .category-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(420px, 1fr));
    gap: 2rem;
  }

  .category-card {
    background-color: var(--card-bg);
    border: 1px solid var(--border-light);
    border-radius: 12px;
    padding: 2rem;
    transition: transform 0.22s cubic-bezier(0.4, 0, 0.2, 1), 
                border-color 0.22s ease, 
                box-shadow 0.22s ease;
    display: flex;
    flex-direction: column;
    scroll-margin-top: 6rem; /* Ensures sticky nav doesn't cover card when scrolling to anchor */
  }

  .category-card:hover {
    transform: translateY(-4px);
    border-color: var(--accent);
    box-shadow: 0 12px 24px -10px rgba(56, 189, 248, 0.15);
  }

  .category-card-header {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    margin-bottom: 1.5rem;
    padding-bottom: 0.75rem;
    border-bottom: 1px solid var(--border-light);
  }

  .category-card-header h2 {
    font-size: 1.45rem;
    color: var(--text-main);
    font-weight: 700;
    margin: 0;
  }

  .category-icon {
    font-size: 1.6rem;
  }

  .doc-count {
    font-size: 0.75rem;
    font-family: var(--font-mono);
    color: var(--accent);
    background-color: rgba(56, 189, 248, 0.08);
    border: 1px solid rgba(56, 189, 248, 0.15);
    padding: 0.2rem 0.6rem;
    border-radius: 20px;
    margin-left: auto;
  }

  .home-docs-list {
    list-style-type: none;
    padding-left: 0;
    margin: 0;
  }

  .home-docs-list li {
    padding: 0.65rem 0;
    border-bottom: 1px solid rgba(255, 255, 255, 0.02);
    display: flex;
    flex-direction: column;
    gap: 0.3rem;
  }

  .home-docs-list li:last-child {
    border-bottom: none;
  }

  .home-docs-list li a {
    color: var(--text-main);
    text-decoration: none;
    font-size: 1.05rem;
    transition: color 0.18s;
  }

  .home-docs-list li a:hover {
    color: var(--accent);
  }

  .doc-meta-badges {
    display: flex;
    flex-wrap: wrap;
    gap: 0.4rem;
    align-items: center;
  }

  .home-subcategory-section {
    margin-top: 1.5rem;
    border-left: 2px solid var(--border-light);
    padding-left: 1rem;
    margin-bottom: 0.5rem;
  }

  .home-subcategory-title {
    font-size: 0.85rem;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--text-muted);
    margin-bottom: 0.6rem;
    font-family: var(--font-mono);
  }

  /* Responsive styling */
  @media (max-width: 900px) {
    .category-grid {
      grid-template-columns: 1fr;
    }
  }
</style>
