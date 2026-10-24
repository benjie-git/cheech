#!/usr/bin/env python3
"""Generate a Chinese-checkers board SVG using the cheech board geometry.

The lattice and starting areas are taken from the game core / iOS client:

  * 13 columns by 19 rows of lattice cells, 247 cells, 121 playable holes.
  * Hole (column, row) is centred at
        x = column + (0.5 if row is even else 0)
        y = row * sqrt(3) / 2
  * Each hole connects to its three forward neighbours, with the same
    distance guard as GameBoard::init_neighbors()/BoardGeometry.edges.
  * The six 10-hole starting triangles are given by BOARD_MAP, using the
    same corner numbering as GameBoard::GOAL_MAP.

The six triangle colours are supplied in clockwise order starting at the
top: top, upper-right, lower-right, bottom, lower-left, upper-left.  Use
"none" for a triangle with no player.

Examples
--------
    # Standard six-player board
    ./tools/build_board_svg.py -o board.svg

    # Standard three-player seating
    ./tools/build_board_svg.py --players 3 -o three.svg

    # Three players in the top, lower-right and lower-left triangles
    ./tools/build_board_svg.py --colors red,none,blue,none,green,none -o three.svg

    # Empty board, exact 1200x1200 raster, larger pegs
    ./tools/build_board_svg.py --players 0 --size 1200x1200 \
        --peg-size 0.46 -o empty.svg

    # Flat app-icon-style square: no board slab, solid background
    ./tools/build_board_svg.py --flat '#f5f0e8' \
        --size 1024x1024 -o icon.svg

    # Hexagonal board with rounded corners around the star
    ./tools/build_board_svg.py --hex-board \
        --size 1024x1024 -o hex.svg

    # Named triangles, custom hex colours and a transparent canvas
    ./tools/build_board_svg.py --top '#e20000' --bottom '#0095e2' \
        --upper-right none --lower-right none \
        --lower-left none --upper-left none -o two.svg

Requires only the Python standard library (or use rsvg-convert to rasterise).

# Icon created using:
./tools/build_board_svg.py --players 3 --colors none,blue,none,green,none,red --hole-size 0.3 --hole-color "#952" --lattice-color "#b74" --padding 1.6 --size 1024x1024 --hex-board --board-color '#c38858' -o board3.svg

"""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Board constants, mirrored from src/game_board.{hh,cc}
# --------------------------------------------------------------------------

SIZE_X = 13
SIZE_Y = 19
COUNT = SIZE_X * SIZE_Y
SQRT3_2 = math.sqrt(3.0) / 2.0

# One row per lattice row.  -1 marks a cell outside the hexagram; the other
# values are the game's two-digit BOARD_MAP codes:
#     value // 10 = goal corner, value % 10 = start corner.
_BOARD_ROWS = (
    (-1,) * 13,
    (-1,) * 6 + (14,) + (-1,) * 6,
    (-1,) * 5 + (14, 14) + (-1,) * 6,
    (-1,) * 5 + (14, 14, 14) + (-1,) * 5,
    (-1,) * 4 + (14, 14, 14, 14) + (-1,) * 5,
    (63,) * 4 + (0,) * 5 + (25,) * 4,
    (63,) * 3 + (0,) * 6 + (25,) * 3 + (-1,),
    (-1,) + (63,) * 2 + (0,) * 7 + (25,) * 2 + (-1,),
    (-1,) + (63,) + (0,) * 8 + (25,) + (-1,) * 2,
    (-1,) * 2 + (0,) * 9 + (-1,) * 2,
    (-1,) + (52,) + (0,) * 8 + (36,) + (-1,) * 2,
    (-1,) + (52,) * 2 + (0,) * 7 + (36,) * 2 + (-1,),
    (52,) * 3 + (0,) * 6 + (36,) * 3 + (-1,),
    (52,) * 4 + (0,) * 5 + (36,) * 4,
    (-1,) * 4 + (41,) * 4 + (-1,) * 5,
    (-1,) * 5 + (41,) * 3 + (-1,) * 5,
    (-1,) * 5 + (41,) * 2 + (-1,) * 6,
    (-1,) * 6 + (41,) + (-1,) * 6,
    (-1,) * 13,
)
BOARD_MAP = tuple(value for row in _BOARD_ROWS for value in row)

# START_MAP[num_players][start_corner] -> player number, or 0 for no player.
START_MAP = {
    1: (0, 1, 0, 0, 0, 0, 0),
    2: (0, 1, 0, 0, 2, 0, 0),
    3: (0, 1, 0, 2, 0, 3, 0),
    4: (0, 0, 1, 2, 0, 3, 4),
    5: (0, 0, 1, 2, 3, 4, 5),
    6: (0, 1, 2, 3, 4, 5, 6),
}

# Physical triangle number (goal corner) -> start corner on the opposite side.
# This is why, for example, the top triangle's BOARD_MAP code is 14: it is
# start corner 4 and goal corner 1.
PHYSICAL_TO_START = {1: 4, 2: 5, 3: 6, 4: 1, 5: 2, 6: 3}

TRIANGLE_NAMES = (
    "top",
    "upper-right",
    "lower-right",
    "bottom",
    "lower-left",
    "upper-left",
)

# Fill and rim colours from GameImages (src/game_images.cc).  The same eight
# names are used throughout the desktop and iOS clients.
APP_COLORS = {
    "red": ("#e20000", "#702323"),
    "orange": ("#fc8e00", "#b66700"),
    "yellow": ("#f6f300", "#6e641e"),
    "green": ("#00d600", "#008c00"),
    "blue": ("#0095e2", "#235671"),
    "purple": ("#b500e2", "#760094"),
    "black": ("#202020", "#1a1a1a"),
    "white": ("#f7f7f7", "#bfbfbf"),
}

# A high-contrast default palette.  Override it with --palette.
DEFAULT_PALETTE = "red,blue,green,orange,purple,yellow"

Color = tuple[str, Optional[str]]  # (fill, rim); rim None means neutral rim


# ---------------------------------------------------------------------------
# Geometry
# ---------------------------------------------------------------------------


def base_point(index: int) -> tuple[float, float]:
    """Untransformed lattice position of a hole, in lattice units."""
    row, column = divmod(index, SIZE_X)
    x = column + (0.5 if row % 2 == 0 else 0.0)
    y = row * SQRT3_2
    return x, y


def present_holes() -> list[int]:
    return [i for i, value in enumerate(BOARD_MAP) if value >= 0]


def lattice_edges() -> list[tuple[int, int]]:
    """Forward-neighbour edges, matching GameBoard::init_neighbors()."""
    edges = []
    for i in present_holes():
        row = i // SIZE_X
        if row % 2 == 0:
            candidates = (i + 1, i + SIZE_X + 1, i + SIZE_X)
        else:
            candidates = (i + 1, i + SIZE_X, i + SIZE_X - 1)
        ax, ay = base_point(i)
        for n in candidates:
            if n < 0 or n >= COUNT or BOARD_MAP[n] < 0:
                continue
            bx, by = base_point(n)
            if math.hypot(ax - bx, ay - by) < 1.1:
                edges.append((i, n))
    return edges


def rotated_points(degrees: float) -> dict[int, tuple[float, float]]:
    """Present-hole positions, rotated about the centre of the hole field."""
    holes = present_holes()
    raw = {i: base_point(i) for i in holes}
    if not degrees:
        return raw

    xs = [p[0] for p in raw.values()]
    ys = [p[1] for p in raw.values()]
    cx = (min(xs) + max(xs)) / 2.0
    cy = (min(ys) + max(ys)) / 2.0
    angle = math.radians(degrees)
    c, s = math.cos(angle), math.sin(angle)

    result = {}
    for i, (x, y) in raw.items():
        dx, dy = x - cx, y - cy
        result[i] = (cx + dx * c - dy * s, cy + dx * s + dy * c)
    return result


def convex_hull(points: dict[int, tuple[float, float]]) -> list[tuple[float, float]]:
    """Convex hull of the hole centres, in order around the perimeter."""
    pts = sorted(set(points.values()))
    if len(pts) <= 2:
        return pts

    def cross(
        origin: tuple[float, float],
        a: tuple[float, float],
        b: tuple[float, float],
    ) -> float:
        return (
            (a[0] - origin[0]) * (b[1] - origin[1])
            - (a[1] - origin[1]) * (b[0] - origin[0])
        )

    lower: list[tuple[float, float]] = []
    for point in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], point) <= 0:
            lower.pop()
        lower.append(point)

    upper: list[tuple[float, float]] = []
    for point in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], point) <= 0:
            upper.pop()
        upper.append(point)

    return lower[:-1] + upper[:-1]


def hex_board_vertices(
    points: dict[int, tuple[float, float]], padding: float
) -> list[tuple[float, float]]:
    """The six star points pushed outwards by padding, as a hexagon."""
    hull = convex_hull(points)
    if len(hull) != 6:
        raise ValueError("a hex board requires six outer star points")

    cx = sum(x for x, _ in hull) / 6.0
    cy = sum(y for _, y in hull) / 6.0
    vertices = []
    for x, y in hull:
        dx, dy = x - cx, y - cy
        distance = math.hypot(dx, dy)
        if distance == 0:
            vertices.append((x, y))
            continue
        scale = (distance + padding) / distance
        vertices.append((cx + dx * scale, cy + dy * scale))
    return vertices


def default_triangle_colors(num_players: int, palette: list[Color]) -> list[Color | None]:
    """Standard seating for num_players, using the core's START_MAP."""
    colors: list[Color | None] = [None] * 6
    if num_players == 0:
        return colors
    if num_players not in START_MAP:
        raise ValueError("players must be between 0 and 6")

    for physical in range(1, 7):
        player = START_MAP[num_players][PHYSICAL_TO_START[physical]]
        if player:
            colors[physical - 1] = palette[player - 1]
    return colors


# ---------------------------------------------------------------------------
# Colours
# ---------------------------------------------------------------------------


def _hex_channels(value: str) -> tuple[int, int, int] | None:
    text = value.strip().lstrip("#")
    if len(text) == 3:
        text = "".join(ch * 2 for ch in text)
    if len(text) != 6 or any(ch not in "0123456789abcdefABCDEF" for ch in text):
        return None
    return tuple(int(text[i:i + 2], 16) for i in (0, 2, 4))  # type: ignore[return-value]


def _darken(value: str, factor: float) -> str:
    channels = _hex_channels(value)
    if channels is None:
        return "#000000"
    return "#%02x%02x%02x" % tuple(
        max(0, min(255, round(channel * factor))) for channel in channels
    )


def parse_color(value: str, edge_darken: float = 0.55) -> Color | None:
    """Parse a colour name, hex value, or 'none'."""
    text = value.strip()
    if text.lower() in {"", "-", "none", "null", "transparent"}:
        return None

    key = text.lower()
    if key in APP_COLORS:
        return APP_COLORS[key]

    channels = _hex_channels(text)
    if channels is not None:
        fill = "#%02x%02x%02x" % channels
        return fill, _darken(fill, edge_darken)

    # Pass other SVG/CSS colour names through; they get a neutral dark rim.
    return text, None


def parse_color_list(value: str, edge_darken: float) -> list[Color | None]:
    return [parse_color(part, edge_darken) for part in value.split(",")]


# ---------------------------------------------------------------------------
# SVG generation
# ---------------------------------------------------------------------------


def _num(value: float) -> str:
    text = f"{value:.3f}".rstrip("0").rstrip(".")
    return "0" if text in {"", "-0"} else text


def _rounded_polygon_path(
    vertices: list[tuple[float, float]], radius: float
) -> str:
    """Closed path with quadratic-rounded corners."""
    if len(vertices) < 3:
        return ""
    if radius <= 0:
        return "M" + " L".join(
            f"{_num(x)} {_num(y)}" for x, y in vertices
        ) + " Z"

    commands: list[str] = []
    count = len(vertices)
    for index, (x, y) in enumerate(vertices):
        px, py = vertices[index - 1]
        nx, ny = vertices[(index + 1) % count]
        pvx, pvy = px - x, py - y
        nvx, nvy = nx - x, ny - y
        plen = math.hypot(pvx, pvy)
        nlen = math.hypot(nvx, nvy)
        if plen == 0 or nlen == 0:
            continue

        dot = (pvx * nvx + pvy * nvy) / (plen * nlen)
        angle = math.acos(max(-1.0, min(1.0, dot)))
        tangent = (
            radius / math.tan(angle / 2.0) if angle > 1e-9 else 0.0
        )
        tangent = min(tangent, 0.5 * plen, 0.5 * nlen)

        start_x = x + pvx / plen * tangent
        start_y = y + pvy / plen * tangent
        end_x = x + nvx / nlen * tangent
        end_y = y + nvy / nlen * tangent

        command = "M" if not commands else "L"
        commands.append(f"{command}{_num(start_x)} {_num(start_y)}")
        if tangent > 0:
            commands.append(
                f"Q{_num(x)} {_num(y)} {_num(end_x)} {_num(end_y)}"
            )
    if not commands:
        return ""
    commands.append("Z")
    return " ".join(commands)


def build_svg(args: argparse.Namespace, triangle_colors: list[Color | None]) -> str:
    points = rotated_points(args.rotation)
    edges = lattice_edges()

    use_hex_board = (
        args.hex_board
        and not args.no_board
        and args.board_color.lower() not in {"none", ""}
    )
    hex_board_path: Optional[str] = None
    if use_hex_board:
        hex_vertices = hex_board_vertices(points, args.padding)
        slab_x = min(x for x, _ in hex_vertices)
        slab_y = min(y for _, y in hex_vertices)
        slab_w = max(x for x, _ in hex_vertices) - slab_x
        slab_h = max(y for _, y in hex_vertices) - slab_y
        hex_board_path = _rounded_polygon_path(
            hex_vertices, max(0.0, args.corner_radius)
        )
    else:
        xs = [p[0] for p in points.values()]
        ys = [p[1] for p in points.values()]
        min_x, max_x = min(xs), max(xs)
        min_y, max_y = min(ys), max(ys)
        slab_x = min_x - args.padding
        slab_y = min_y - args.padding
        slab_w = (max_x - min_x) + 2.0 * args.padding
        slab_h = (max_y - min_y) + 2.0 * args.padding

    radius = max(0.0, min(args.corner_radius, slab_w / 2.0, slab_h / 2.0))

    view_x = slab_x - args.margin
    view_y = slab_y - args.margin
    view_w = slab_w + 2.0 * args.margin
    view_h = slab_h + 2.0 * args.margin

    if args.size:
        width, height = args.size
    else:
        width = max(1, math.ceil(view_w * args.unit))
        height = max(1, math.ceil(view_h * args.unit))

    # Expand the viewBox so it has exactly the requested pixel aspect ratio.
    # This keeps --canvas covering the whole image instead of leaving a
    # letterbox outside the viewBox.
    target_aspect = width / height
    view_aspect = view_w / view_h
    if view_aspect < target_aspect:
        expanded_w = view_h * target_aspect
        view_x -= (expanded_w - view_w) / 2.0
        view_w = expanded_w
    elif view_aspect > target_aspect:
        expanded_h = view_w / target_aspect
        view_y -= (expanded_h - view_h) / 2.0
        view_h = expanded_h

    out: list[str] = []
    add = out.append

    add(
        '<svg xmlns="http://www.w3.org/2000/svg" '
        f'width="{width}" height="{height}" '
        f'viewBox="{_num(view_x)} {_num(view_y)} {_num(view_w)} {_num(view_h)}" '
        'preserveAspectRatio="xMidYMid meet">'
    )
    add("<title>Chinese Checkers board</title>")
    add(
        "<desc>"
        + " ".join(
            f"{name}={color[0] if color else 'none'}"
            for name, color in zip(TRIANGLE_NAMES, triangle_colors)
        )
        + "</desc>"
    )

    # Peg templates, one per distinct colour.
    peg_colors: list[Color] = []
    peg_ids: dict[Color, int] = {}
    for color in triangle_colors:
        if color is not None and color not in peg_ids:
            peg_ids[color] = len(peg_colors)
            peg_colors.append(color)

    if peg_colors:
        add("<defs>")
        for color, peg_id in peg_ids.items():
            fill, rim = color
            add(f'<g id="peg{peg_id}">')
            if rim is not None:
                add(f'<circle r="{_num(args.peg_size)}" fill="{rim}"/>')
            else:
                add(
                    f'<circle r="{_num(args.peg_size)}" fill="#000000" '
                    'fill-opacity="0.45"/>'
                )
            add(
                f'<circle r="{_num(args.peg_size * args.peg_inner)}" '
                f'fill="{fill}"/>'
            )
            if args.gloss > 0:
                r = args.peg_size
                add(
                    f'<ellipse cx="{_num(-0.10 * r)}" cy="{_num(-0.325 * r)}" '
                    f'rx="{_num(0.35 * r)}" ry="{_num(0.275 * r)}" '
                    f'fill="#ffffff" fill-opacity="{_num(args.gloss)}"/>'
                )
            add("</g>")
        add("</defs>")

    if args.canvas.lower() not in {"none", ""}:
        add(
            f'<rect id="background" x="{_num(view_x)}" y="{_num(view_y)}" '
            f'width="{_num(view_w)}" height="{_num(view_h)}" '
            f'fill="{args.canvas}"/>'
        )

    # Board slab: a rounded rectangle hugging the lattice by default, or a
    # rounded hexagon around the six star points with --hex-board.
    if not args.no_board and args.board_color.lower() not in {"none", ""}:
        outline = (
            f' stroke="{args.board_outline}" '
            f'stroke-width="{_num(args.board_outline_width)}" '
            'stroke-linejoin="round"'
            if args.board_outline.lower() not in {"none", ""}
            else ""
        )
        if hex_board_path is not None:
            add(
                f'<path id="board" d="{hex_board_path}" '
                f'fill="{args.board_color}"{outline}/>'
            )
        else:
            add(
                f'<rect id="board" x="{_num(slab_x)}" y="{_num(slab_y)}" '
                f'width="{_num(slab_w)}" height="{_num(slab_h)}" '
                f'rx="{_num(radius)}" ry="{_num(radius)}" '
                f'fill="{args.board_color}"{outline}/>'
            )

    # Lattice edges.
    if not args.no_lattice and args.lattice_color.lower() not in {"none", ""}:
        path = []
        for a, b in edges:
            ax, ay = points[a]
            bx, by = points[b]
            path.append(f"M{_num(ax)} {_num(ay)}L{_num(bx)} {_num(by)}")
        add(
            f'<path d="{" ".join(path)}" fill="none" '
            f'stroke="{args.lattice_color}" '
            f'stroke-width="{_num(args.lattice_width)}" '
            'stroke-linecap="round"/>'
        )

    # Empty holes.
    if not args.no_holes and args.hole_color.lower() not in {"none", ""}:
        add(
            f'<g id="holes" fill="{args.hole_color}"'
            + (
                f' stroke="{args.hole_outline}" '
                f'stroke-width="{_num(args.hole_outline_width)}"'
                if args.hole_outline.lower() not in {"none", ""}
                else ""
            )
            + ">"
        )
        for i in present_holes():
            x, y = points[i]
            add(
                f'<circle cx="{_num(x)}" cy="{_num(y)}" '
                f'r="{_num(args.hole_size)}"/>'
            )
        add("</g>")

    # Initial pegs: every hole in a coloured starting triangle gets a peg.
    if peg_colors:
        add('<g id="pegs">')
        for i in present_holes():
            physical = BOARD_MAP[i] // 10
            if physical == 0:
                continue
            color = triangle_colors[physical - 1]
            if color is None:
                continue
            x, y = points[i]
            add(f'<use href="#peg{peg_ids[color]}" x="{_num(x)}" y="{_num(y)}"/>')
        add("</g>")

    add("</svg>")
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------
# Command line
# ---------------------------------------------------------------------------


def _size(value: str) -> tuple[int, int]:
    for separator in ("x", "X", ","):
        if separator in value:
            left, right = value.split(separator, 1)
            break
    else:
        raise argparse.ArgumentTypeError("size must be WIDTHxHEIGHT")
    try:
        width, height = int(left), int(right)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("size must be WIDTHxHEIGHT") from exc
    if width <= 0 or height <= 0:
        raise argparse.ArgumentTypeError("size values must be positive")
    return width, height


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        description="Generate a cheech Chinese-checkers board as an SVG.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Triangle colours are clockwise from the top:\n"
            "  top, upper-right, lower-right, bottom, lower-left, upper-left\n"
            "Use 'none' for an empty triangle.  A colour may be one of the\n"
            "eight app colour names, a #rrggbb value, or any SVG colour name.\n"
            "Sizes in lattice units are radii; the hole spacing is one unit."
        ),
    )
    ap.add_argument("-o", "--output", default="board.svg",
                    help="output SVG path (default: board.svg; use '-' for stdout)")

    seating = ap.add_argument_group("seating")
    seating.add_argument(
        "--players", type=int, metavar="N",
        help="standard N-player seating (0-6; default: 6); ignored when --colors is used",
    )
    seating.add_argument(
        "--palette", default=DEFAULT_PALETTE, metavar="C1,C2,...",
        help=f"colours for players 1..N with --players (default: {DEFAULT_PALETTE})",
    )
    seating.add_argument(
        "--colors", metavar="C1,C2,C3,C4,C5,C6",
        help="six triangle colours, clockwise from the top; shorter lists are padded with none (overrides --players)",
    )
    for name in TRIANGLE_NAMES:
        seating.add_argument(
            f"--{name}", dest=name.replace("-", "_"), metavar="COLOR",
            help=f"override the colour of the {name} triangle",
        )

    layout = ap.add_argument_group("layout and size")
    layout.add_argument("--size", type=_size, metavar="WxH",
                        help="SVG width/height in pixels; default fits --unit")
    layout.add_argument("--unit", type=float, default=48.0, metavar="PX",
                        help="pixels per lattice step when --size is omitted (default: 48)")
    layout.add_argument("--margin", type=float, default=0.15, metavar="U",
                        help="extra space outside the board slab, in lattice units (default: 0.15)")
    layout.add_argument("--padding", type=float, default=0.8, metavar="U",
                        help=(
                            "space beyond the outermost hole centres; with "
                            "--hex-board, the radial gap to the hexagon "
                            "corners (default: 0.8)"
                        ))
    layout.add_argument("--corner-radius", type=float, default=0.8, metavar="U",
                        help="board slab corner radius (default: 0.8)")
    layout.add_argument("--rotation", type=float, default=0.0, metavar="DEG",
                        help="clockwise board rotation in degrees (default: 0)")

    style = ap.add_argument_group("style")
    style.add_argument("--canvas", "--background", dest="canvas",
                       default="none", metavar="COLOR",
                       help="whole-image background colour; fills the entire SVG (default: none)")
    style.add_argument("--flat", metavar="COLOR",
                       help="shorthand for --no-board --background COLOR")
    style.add_argument("--board-color", default="#8c663d", metavar="COLOR",
                       help="board slab colour (default: #8c663d; 'none' also omits it)")
    style.add_argument("--no-board", action="store_true",
                       help="omit the rounded board slab and draw the lattice/holes/pegs directly")
    style.add_argument("--hex-board", "--hexagonal-board", dest="hex_board",
                       action="store_true",
                       help="draw the board slab as a rounded hexagon around the six star points")
    style.add_argument("--board-outline", default="none", metavar="COLOR",
                       help="optional board slab outline (default: none)")
    style.add_argument("--board-outline-width", type=float, default=0.04, metavar="U",
                       help="board slab outline width (default: 0.04)")
    style.add_argument("--lattice-color", default="#7d5733", metavar="COLOR",
                       help="lattice line colour (default: #7d5733)")
    style.add_argument("--lattice-width", type=float, default=0.06, metavar="U",
                       help="lattice line width (default: 0.06)")
    style.add_argument("--no-lattice", action="store_true",
                       help="do not draw the lattice lines")
    style.add_argument("--hole-color", default="#664226", metavar="COLOR",
                       help="empty-hole colour (default: #664226)")
    style.add_argument("--hole-size", type=float, default=0.38, metavar="U",
                       help="empty-hole radius in lattice units (default: 0.38)")
    style.add_argument("--hole-outline", default="none", metavar="COLOR",
                       help="optional empty-hole outline (default: none)")
    style.add_argument("--hole-outline-width", type=float, default=0.02, metavar="U",
                       help="empty-hole outline width (default: 0.02)")
    style.add_argument("--no-holes", action="store_true",
                       help="do not draw the empty holes")
    style.add_argument("--peg-size", type=float, default=0.42, metavar="U",
                       help="peg radius in lattice units (default: 0.42)")
    style.add_argument("--peg-inner", type=float, default=0.88, metavar="FRACTION",
                       help="coloured peg face radius as a fraction of --peg-size (default: 0.88)")
    style.add_argument("--peg-edge", default="auto", metavar="COLOR",
                       help="override every peg rim colour (default: auto)")
    style.add_argument("--edge-darken", type=float, default=0.55, metavar="FACTOR",
                       help="rim brightness for custom hex colours (default: 0.55)")
    style.add_argument("--gloss", type=float, default=0.35, metavar="OPACITY",
                       help="peg highlight opacity, 0 to disable (default: 0.35)")
    style.add_argument("--no-gloss", action="store_true",
                       help="do not draw peg highlights")
    return ap


def _resolve_colours(args: argparse.Namespace) -> list[Color | None]:
    if args.edge_darken < 0 or args.edge_darken > 1:
        raise SystemExit("--edge-darken must be between 0 and 1")
    if args.peg_inner < 0 or args.peg_inner > 1:
        raise SystemExit("--peg-inner must be between 0 and 1")
    if args.gloss < 0 or args.gloss > 1:
        raise SystemExit("--gloss must be between 0 and 1")

    palette = parse_color_list(args.palette, args.edge_darken)
    if args.players is None:
        num_players = 6
    else:
        if args.players < 0 or args.players > 6:
            raise SystemExit("--players must be between 0 and 6")
        num_players = args.players

    if args.colors is not None:
        colors = parse_color_list(args.colors, args.edge_darken)
        if len(colors) > 6:
            raise SystemExit("--colors accepts at most six colours")
        colors.extend([None] * (6 - len(colors)))
    elif args.players is not None or not _has_triangle_override(args):
        if num_players > len(palette):
            raise SystemExit("--palette needs at least one colour per player")
        colors = default_triangle_colors(num_players, palette)
    else:
        colors = [None] * 6

    for index, name in enumerate(TRIANGLE_NAMES):
        raw = getattr(args, name.replace("-", "_"))
        if raw is not None:
            colors[index] = parse_color(raw, args.edge_darken)

    if args.peg_edge.lower() != "auto":
        edge = parse_color(args.peg_edge, args.edge_darken)
        if edge is None:
            raise SystemExit("--peg-edge cannot be none")
        colors = [(color[0], edge[0]) if color else None for color in colors]
    return colors


def _has_triangle_override(args: argparse.Namespace) -> bool:
    return any(
        getattr(args, name.replace("-", "_")) is not None
        for name in TRIANGLE_NAMES
    )


def main(argv: list[str] | None = None) -> int:
    ap = build_parser()
    args = ap.parse_args(argv)

    if args.flat is not None:
        args.no_board = True
        args.canvas = args.flat

    for name in ("padding", "margin", "corner_radius",
                 "lattice_width", "hole_size", "peg_size",
                 "board_outline_width", "hole_outline_width"):
        if getattr(args, name) < 0:
            ap.error(f"--{name.replace('_', '-')} must not be negative")
    if args.unit <= 0:
        ap.error("--unit must be positive")
    if args.peg_size <= 0:
        ap.error("--peg-size must be positive")
    if args.hole_size < 0:
        ap.error("--hole-size must not be negative")
    if args.no_gloss:
        args.gloss = 0.0

    triangle_colors = _resolve_colours(args)
    svg = build_svg(args, triangle_colors)

    if args.output == "-":
        sys.stdout.write(svg)
    else:
        path = Path(args.output)
        path.write_text(svg, encoding="utf-8")
        print(f"wrote {path} ({len(svg)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
