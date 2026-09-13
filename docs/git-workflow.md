# Git en AgroNexus

- `git status --short --branch`: muestra rama y cambios pendientes, sin modificar archivos.
- `git diff`: revisa cambios de archivos ya versionados; no muestra el contenido de archivos nuevos sin seguimiento.
- `git ls-files --others --exclude-standard`: lista archivos nuevos que Git podría incorporar.
- `git check-ignore -v RUTA`: explica qué regla excluye un archivo.

`git add` prepara archivos para un commit; no publica nada. `git commit` crea una versión local. `git push` publica commits al remoto. Los commits y push requieren una indicación explícita del usuario.

Antes de preparar archivos, revisar contenidos y exclusiones. Evitar `git add -f`: puede incorporar secretos y originales ignorados. `.gitignore` no elimina archivos ni deja de seguir archivos ya versionados.

`legacy_code/`, `secrets.local.h`, `.env`, `.env.*` y resultados de compilación están ignorados. Los originales se mantienen localmente; GitHub no será su respaldo.
