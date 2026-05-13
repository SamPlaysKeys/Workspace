---
layout: page
title: Writing
permalink: /blog/
---

Notes and drafts published as posts. For repository-only blog folder notes, see the [blog](https://github.com/SamPlaysKeys/Workspace/tree/main/docs/blog) directory on GitHub.

{% for post in site.posts %}
- **{{ post.date | date: site.minima.date_format }}** — [{{ post.title }}]({{ post.url | relative_url }})
{% endfor %}
