"""Builds the playable web prototype from the haul's tables: python3 prototypes/web/build.py"""
import json
import os

here = os.path.dirname(os.path.abspath(__file__))
tables = json.load(open(os.path.join(here, "..", "haul", "tables.json")))
html = open(os.path.join(here, "template.html")).read().replace("/*TABLES*/", json.dumps(tables))
open(os.path.join(here, "spintacular-haul.html"), "w").write(html)
print("wrote prototypes/web/spintacular-haul.html")
