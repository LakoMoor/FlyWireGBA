"""Convert actual mGBA captures into deliverable screenshots/GIF (requires Pillow)."""
from pathlib import Path
from PIL import Image, ImageOps, ImageDraw
out=Path('docs/screenshots');out.mkdir(parents=True,exist_ok=True)
for source in sorted(Path('build').glob('emulator-*.ppm')):
 Image.open(source).resize((720,480),Image.Resampling.NEAREST).save(out/(source.stem.removeprefix('emulator-')+'.png'))
frames=[Image.open(p).resize((720,480),Image.Resampling.NEAREST) for p in sorted(Path('build').glob('demo-*.ppm'))]
if frames:frames[0].save('docs/demo.gif',save_all=True,append_images=frames[1:],duration=100,loop=0,optimize=True)
canvas=Image.new('RGB',(976,656),(8,13,20))
for i,name in enumerate(['arena','flight','brain','stats']):
 im=Image.open(out/(name+'.png')).resize((480,320),Image.Resampling.NEAREST)
 canvas.paste(im,(8+(i%2)*488,8+(i//2)*328))
canvas.save('docs/preview.png')
for view in range(3):
 sources=list(Path('build').glob(f'model-{view}-*.ppm'))
 if not sources:continue
 sheet=Image.new('RGB',(178*8,118*12),(12,19,26))
 for state in range(6):
  for direction in range(16):
   im=Image.open(f'build/model-{view}-{state}-{direction:02d}.ppm')
   sheet.paste(im,((direction%8)*178,(state*2+direction//8)*118))
 sheet.save(f'docs/body-turntable-{view}.png')

pet_frames=[Image.open(p).resize((720,480),Image.Resampling.NEAREST) for p in sorted(Path("build").glob("pet-*.ppm"))]
if pet_frames:pet_frames[0].save("docs/pet.gif",save_all=True,append_images=pet_frames[1:],duration=100,loop=0,optimize=True)
